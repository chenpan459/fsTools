# e2fsck 目录代码功能说明（e2fsprogs 1.46.5）

本文归纳 **`e2fsprogs-1.46.5/e2fsck/`**：**`e2fsck`**（ext2/ext3/ext4 文件系统一致性检查与修复）的 C 源码布局。实现以 **`libext2fs`** 为核心，配合 **`libsupport`、e2p、blkid、uuid、com_err** 等；元数据与错误报告由 **`e2fsck.h`** 中的上下文 **`e2fsck_t`**（即 **`struct e2fsck_struct`**）贯穿各遍（pass）。

**路径**：`../e2fsprogs-1.46.5/e2fsck/`

---

## 1. 构建与依赖（`Makefile.in` 摘要）

| 项目 | 说明 |
|------|------|
| **主程序** | **`e2fsck`** 由 **`OBJS`** 中列出的 `.o` 链接 **`LIBS`** 得到。 |
| **LIBS** | **`$(LIBSUPPORT) $(LIBEXT2FS) $(LIBCOM_ERR) $(LIBBLKID) $(LIBUUID) $(LIBINTL) $(LIBE2P) $(LIBMAGIC) $(SYSLIBS)`** — 具体组成随 **`configure`**（如 quota、magic）变化。 |
| **手册** | **`e2fsck.8`**、**`e2fsck.conf.5`**。 |
| **其它目标** | **`e2fsck.static` / `e2fsck.profiled`**；独立工具 **`iscan`、`extend`、`flushb`** 等**不**并入 **`e2fsck`** 主二进制（见末节）。 |
| **问题码** | **`problem.h` / `problemP.h`** 定义 **`PR_*`** 与 **`fix_problem`** 行为（见 **`problem.c`**）。 |

---

## 2. 执行主路径（谁在调谁）

1. **入口**：**`unix.c`** 中 **`main`** — 解析 **`-p/-y/-f/...`、`-b` superblock、`-j` 外部日志、`-E` 扩展选项、`-z` undo** 等，打开设备，**`plausible`/`blkid`** 一类检查，分配 **`e2fsck` 上下文**。  
2. **日志重放**：对已含 journal 的卷，在适当时机调用 **`e2fsck_run_ext3_journal`**（**`journal.c`**），依赖 **`recovery.c` / `revoke.c`** 中与 **JBD2** 用户态等价逻辑。  
3. **多遍检查**：**`e2fsck.c`** 里 **`e2fsck_run`** 顺序调用（见下节 **`e2fsck_passes[]`**）。  
4. **交互与退出码**：**`problem.c` + `message.c`** 驱动 **`fix_problem`/提示**；**`e2fsck.h`** 中定义 **`FSCK_*`** 退出码约定。

**`e2fsck_allocate_context` / `e2fsck_free_context` / `e2fsck_reset_context`**（**`e2fsck.c`**）管理位图、**`dir_info`、extent 待重建列表、加密元数据缓存**等生命周期。

---

## 3. 标准遍次（`e2fsck_run` 中的数组）

**`e2fsck.c`** 中：

```c
static pass_t e2fsck_passes[] = {
    e2fsck_pass1, e2fsck_pass1e, e2fsck_pass2, e2fsck_pass3,
    e2fsck_pass4, e2fsck_pass5, 0 };
```


| 函数 | 源文件 | 作用概要 |
|------|--------|----------|
| **`e2fsck_pass1`** | **`pass1.c`** | **遍 1**：顺序扫描 **inode 表** — 校验 mode/尺寸/块映射，建立 **inode/块使用位图**、**重复块**、**目录数据收集**、**加密策略/ea_inode** 等；若发现 **重复数据块** 可触发 **pass1B/C/D**（**`pass1b.c`**，不在上表数组内）。 |
| **`e2fsck_pass1e`** | **`extents.c`** | **遍 1E**：对标记需重建的 inode **重建 extent 树**（与 **`E2F_OPT_*`** 选项协同）。 |
| **`e2fsck_pass2`** | **`pass2.c`** | **遍 2**：按块号排序处理 **目录块**，校验 **`ext2_dir_entry`**（**`rec_len`/`name_len`/`.`/`..`** 等），建立子目录关系；释放部分 pass1 侧结构。 |
| **`e2fsck_pass3`** | **`pass3.c`** | **遍 3**：**目录连通性** — 从根追踪 **`dirinfo`**，处理 **断链目录、环**；**`/lost+found`**、**`e2fsck_reconnect_file`**。 |
| **`e2fsck_pass4`** | **`pass4.c`** | **遍 4**：**链接计数**、**断连 inode** 与 **ea_inode 引用** 等收尾。**`disconnect_inode`** 等。 |
| **`e2fsck_pass5`** | **`pass5.c`** | **遍 5**：将 **内存中推导的块/inode 位图** 与 **盘上位图** 对照（含 **checksum** 相关子检查），必要时写回。 |

**补充**：**`super.c`** 在 **各遍之前/之中**做 **superblock、组描述符、特性** 等校验与修正（与 **`fix_problem`** 问题码配合），不一定单独命名为 “pass0”，但属于检查管线的前端。

**`pass1b.c`**：**pass1B**（列举重复块归属）、**pass1C**（找路径名）、**pass1D**（克隆或删除）— **仅在 pass1 发现重复块时调用**。

---

## 4. `OBJS` 与源文件对照（主体库）

以下与 **`Makefile.in`** 中 **`OBJS`** 一致（不含可选 **`mtrace`**）。

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`unix.o`** | **`unix.c`** | **`main`**、参数与环境、打开块设备、调用 **`e2fsck_run`**、进度与摘要输出。 |
| **`e2fsck.o`** | **`e2fsck.c`** | **上下文分配/释放/复位**、**`e2fsck_run`**（**`setjmp`** 中止路径）、pass 调度骨架。 |
| **`super.o`** | **`super.c`** | **Superblock / group descriptor** 合法性检查与常见损坏修复入口。 |
| **`pass1.o`** | **`pass1.c`** | **遍 1** inode 扫描与数据结构构建（见文件头长注释）。 |
| **`pass1b.o`** | **`pass1b.c`** | **重复块** 专用子遍。 |
| **`pass2.o`～`pass5.o`** | **`pass2.c`～`pass5.c`** | **遍 2～5**（见上表）。 |
| **`journal.o`** | **`journal.c`** | **ext3/4 journal**：打开 journal、**checksum**、**`e2fsck_run_ext3_journal`** 等与 **`jfs_user.h`** 衔接。 |
| **`badblocks.o`** | **`badblocks.c`** | **`read_bad_blocks_file`**：读 **`badblocks`** 列表并合并进 **坏块 inode**。 |
| **`util.o`** | **`util.c`** | 杂项：终端交互、时间/资源统计、与 **`e2fsck`** 全局辅助函数。 |
| **`dirinfo.o`** | **`dirinfo.c`** | **`dir_info`** 表：**`..` 与实际父目录** 等；可选用 **TDB** 降内存（**`CONFIG_TDB`**）。 |
| **`dx_dirinfo.o`** | **`dx_dirinfo.c`** | **/htree** 目录的 **`dx_dir_info`**：各目录块类型、hash 范围等元数据。 |
| **`ehandler.o`** | **`ehandler.c`** | **`io_channel`** 读写错误处理：坏块上的重试/跳过策略。 |
| **`problem.o`** | **`problem.c`** | **`fix_problem`**：按 **`problem.h`** 问题码与用户选项（**`-y/-n/-p`**）决定修复动作。 |
| **`message.o`** | **`message.c`** | **`print_e2fsck_message`**：**%b/%i/…** 模板展开（见文件头注释）。 |
| **`quota.o`** | **`quota.c`** | **ext4 quota inode** 相关移动/校验辅助。 |
| **`recovery.o`** | **`recovery.c`** | 来自内核 **jbd2 recovery** 的用户态移植：**journal 重放** 主循环。 |
| **`region.o`** | **`region.c`** | **区间分配器**：跟踪一块逻辑区域内的已分配区间（测试目标 **`tst_region`**）。 |
| **`revoke.o`** | **`revoke.c`** | **Journal revoke** 表：**防止旧日志覆盖新元数据**（与 **`recovery.c`** 配合）。 |
| **`ea_refcount.o`** | **`ea_refcount.c`** | **扩展属性块引用计数** 与 **`block_ea_map`**（**`tst_refcount`**）。 |
| **`rehash.o`** | **`rehash.c`** | **htree 目录重建**（大目录内存与算法说明见文件头）。 |
| **`logfile.o`** | **`logfile.c`** | **`-v`/日志文件**、进度 FD（**`-C`**）等；可带 **`TEST_PROGRAM`** 小 **`main`**。 |
| **`sigcatcher.o`** | **`sigcatcher.c`** | **SIGINT/SIGTERM** 等：**取消/安全退出** 与 **`e2fsck_global_ctx`** 协同。 |
| **`readahead.o`** | **`readahead.c`** | 元数据 **readahead**：按目录块排序预读以加速 fsck。 |
| **`extents.o`** | **`extents.c`** | **`e2fsck_pass1e`**、**`e2fsck_rebuild_extents*`**：extent 树重建与调度。 |
| **`encrypted_files.o`** | **`encrypted_files.c`** | **fscrypt**：pass1 建立 **inode→policy**，pass2 校验策略一致性；**RLE** 等省内存。 |

---

## 5. 头文件与问题码

| 文件 | 作用 |
|------|------|
| **`e2fsck.h`** | **`e2fsck_t`、选项/标志、`dir_info`/`dx_dir_info`、pass 函数声明、与 **quota/fast_commit** 的包含关系。 |
| **`problem.h` / `problemP.h`** | 问题编号 **`PR_*`** 与 **`fix_problem`** 语义（由 **`compile_et`** 链生成/维护）。 |
| **`jfs_user.h`** | journal 代码用户态适配层（与 **`journal.c`/`recovery.c`/`revoke.c`** 一起使用）。 |

---

## 6. 自检与单测程序（`Makefile.in` 节选）

- **`tst_refcount`、`tst_region`、`tst_problem`**：分别链 **ea_refcount、region、problem** 的单元逻辑。  
- **`tst_sigcatcher`**：信号处理调试。  

**`fullcheck`** 会运行上述 **`tst_*`**。

---

## 7. 同目录但未并入 `e2fsck` 主程序的源

用于开发或独立小工具，**默认 OBJS 不含**：**`iscan.c`**（inode 扫描）、**`extend.c`、`flushb.c`、`scantest.c`、`emptydir.c`** 等。需要时在 **Makefile** 中单独 `make` 对应目标。

---

## 8. 推荐阅读顺序

1. **`e2fsck.h`** — 上下文与选项。  
2. **`pass1.c` 文件头注释** — 理解位图与后续遍的输入。  
3. **`e2fsck.c` 中 `e2fsck_run` + `e2fsck_passes[]`** — 固定遍次顺序。  
4. **`problem.h` + `problem.c`** — 自动修复与交互策略。  
5. **`journal.c` 与 `unix.c` 中 `e2fsck_run_ext3_journal` 调用点** — 与只读检查/写回的关系。

---

## 9. 与其它文档的关系

磁盘格式与 **`libext2fs`** API 见 **[libext2fs 代码功能说明](./libext2fs-代码功能说明.md)**；**profile/quota** 与 **[libsupport 代码功能说明](./libsupport-代码功能说明.md)**、**blkid** 与 **[libblkid 代码功能说明](./libblkid-代码功能说明.md)** 可交叉查阅。

---

*文档依据 `e2fsck/Makefile.in`、`e2fsck.h`、`e2fsck.c`、`unix.c` 及各 pass 源文件头注释归纳。*
