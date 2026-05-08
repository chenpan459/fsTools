# misc 目录代码功能说明（e2fsprogs 1.46.5）

本文归纳 **`e2fsprogs-1.46.5/misc/`**：面向最终用户的 **命令行工具与包装程序** 源码树（与 **`e2fsck/`**、**`debugfs/`** 等并列）。**`Makefile.in`** 将目标分为 **`SPROGS`（装到 `sbin` 类）、`USPROGS`、`UPROGS`（`bin`）、`LPROGS`（视配置）**，其中多项随 **`configure`** 的 **`@*_CMT@`** 宏**条件启用**（如 **blkid、uuidd、e2image、e4defrag、fuse2fs、uuidgen**）。

**路径**：`../e2fsprogs-1.46.5/misc/`

---

## 1. 通用链接与共享源码

| 项目 | 说明 |
|------|------|
| **常见静态/共享依赖** | 多数程序链接 **`libext2fs`、`libcom_err`、`libsupport`**（**`LIBS` / `DEPLIBS`**）；**`tune2fs`、`mke2fs`、`dumpe2fs`** 等再链 **`libe2p`、`libblkid`、`libuuid`、`libintl`** 等，见各目标规则。 |
| **`util.c` / `util.h`** | **`mke2fs` 与 `tune2fs`** 共用的杂项（终端列宽、若干辅助）。 |
| **日志子系统对象文件的复用** | **`tune2fs`、`fuse2fs`** 将 **`journal.o`** 编自 **`../debugfs/journal.c`**，**`recovery.o` / `revoke.o`** 编自 **`../e2fsck/recovery.c` 与 `revoke.c`**（见 **`Makefile.in`** 中 **`JOURNAL_CFLAGS`、`DEPEND_CFLAGS`**），避免在 **`misc/`** 下重复维护一份实现。 |
| **`mke2fs` 默认配置进 C** | **`default_profile.c`** 由 **`mke2fs.conf` + `profile-to-c.awk`** 生成，把 **`mke2fs.conf.in`** 规则编译进 **`mke2fs`**。 |
| **手册与配置模板** | 大量 **`.8.in` / `.1.in` / `.5.in`** 经 **`SUBSTITUTE_UPTIME`** 生成安装页；**`mke2fs.conf.in`、ext4.5** 等。 |

---

## 2. 已安装/主流程工具（按功能）

### 2.1 创建与调整文件系统

| 程序 / 链接名 | 源文件 | 功能概要 |
|----------------|--------|----------|
| **`mke2fs` / `mkfs.ext2`…** | **`mke2fs.c`**，外加 **`util.c`、`default_profile.c`、`mk_hugefiles.c`、`create_inode.c`** | **格式化**：读 **profile**、分配块组、写 superblock/GDT；**`-d` 从目录种子建文件系统** 时用 **`create_inode.c`**；**巨型预分配文件** 用 **`mk_hugefiles.c`**。 |
| **`tune2fs`** | **`tune2fs.c`** + **`util.c` + `journal.o` + `recovery.o` + `revoke.o`** | **在线/离线调参数**：特征位、挂载默认、journal 大小与位置、MMP 等；与 **`e2fsck`** 同源 journal 代码路径。 |
| **`e2label`** | **`tune2fs.c`** 内 **`parse_e2label_options`**（**`argv[0]` 识别为 `e2label`**） | **设置/查看卷标** 的薄封装。 |

### 2.2 只读/元数据查看

| 程序 / 链接名 | 源文件 | 功能概要 |
|----------------|--------|----------|
| **`dumpe2fs`** | **`dumpe2fs.c`** | 打印 **superblock、组描述符、位图摘要、特性、journal 位置** 等。 |
| **`e2mmpstatus`** | 安装为指向 **`dumpe2fs`** 的链接；**`dumpe2fs.c`** 中根据 **`program_name` 含 `mmpstatus`** 走 **MMP 状态** 输出。 | **多处主机挂载保护（MMP）** 诊断。 |

### 2.3 坏块与镜像

| 程序 | 源文件 | 功能概要 |
|------|--------|----------|
| **`badblocks`** | **`badblocks.c`** | **线性/随机**读检测坏块，可输出供 **`mke2fs -l`**。 |
| **`e2image`** | **`e2image.c`** | **元数据镜像/备份**（raw/QCOW 等模式，见手册），链 **blkid/magic**。 |

### 2.4 标识、查找与包装 `fsck`

| 程序 | 源文件 | 功能概要 |
|------|--------|----------|
| **`blkid`** | **`blkid.c`** | **`libblkid`** 命令行：列出 **UUID/LABEL/TYPE**。 |
| **`findfs`** | 通常安装为指向 **`blkid`** 的符号链接 | **LABEL=/UUID=** 解析设备节点。 |
| **`fsck`** | **`fsck.c` + `base_device.c` + `ismounted.c`** | **通用 fsck 前端**：解析 **`fstab`、并行调度、**`-t`/`fsck.type`** 分派。 |

### 2.5 属性与 UUID

| 程序 | 源文件 | 功能概要 |
|------|--------|----------|
| **`chattr` / `lsattr`** | **`chattr.c` / `lsattr.c`** | **`libe2p`** + ioctl：**immutable、extent、project** 等标志。 |
| **`uuidgen`** | **`uuidgen.c`** | 生成 **DCE UUID** 字符串（**`libuuid`**）。 |
| **`uuidd`** | **`uuidd.c`** | **基于 socket 的 UUID 时间序批处理**（减轻 **`/dev/urandom`** 压力），常作系统服务。 |

### 2.6 碎片、加密、撤销与 FUSE

| 程序 | 源文件 | 功能概要 |
|------|--------|----------|
| **`filefrag`** | **`filefrag.c`**（**Linux**） | **`FIEMAP`/`ioctl`** 报告 **文件碎片**；非 Linux 下桩为报错退出。 |
| **`e2freefrag`** | **`e2freefrag.c` + `e2freefrag.h`** | **按 free-space extent** 统计**空闲区碎片**。 |
| **`e4defrag`** | **`e4defrag.c`** | **ext4 在线 defrag**（**`DEFRAG_CMT`** 等控制是否编译）。 |
| **`e4crypt`** | **`e4crypt.c`** | **fscrypt 策略/包装**（密钥环、目录加密属性，**`LINUX_CMT`**）。 |
| **`e2undo`** | **`e2undo.c`** | 重放 **`mke2fs`/`tune2fs`/`e2fsck`** 等生成的 **undo 日志**，撤销块写入。 |
| **`fuse2fs`** | **`fuse2fs.c` + journal/recovery/revoke** | **用户态 FUSE** 挂载 ext4（**`FUSE_CMT`**）。 |

### 2.7 其它已纳入 **`all`** 的实用程序

| 程序 | 源文件 | 功能概要 |
|------|--------|----------|
| **`logsave`** | **`logsave.c`** | 在 **`/var/log` 可写前**把子进程输出保存到临时文件，适合启动脚本。 |
| **`mklost+found`** | **`mklost+found.c`** | 预扩容 **`lost+found`** 目录。 |
| **`e2fuzz` / `check_fuzzer`** | **`e2fuzz.c`**；**`check_fuzzer.c`** 另生成独立 **`check_fuzzer`** | **对镜像做伪随机/metadata 损坏** 以测 **e2fsck**；**`check_fuzzer`** 为相关自检小工具。 |

---

## 3. 条件编译与分组变量（`Makefile.in`）

| 变量 | 含义（典型） |
|------|----------------|
| **`@BLKID_CMT@`** | 非空则构建 **`blkid`、`findfs`**。 |
| **`@UUIDD_CMT@`** | **`uuidd`**。 |
| **`@IMAGER_CMT@`** | **`e2image`**。 |
| **`@DEFRAG_CMT@` + `@LINUX_CMT@`** | **`e4defrag`**。 |
| **`@FUSE_CMT@`** | **`fuse2fs`**。 |
| **`@UUID_CMT@`** | **`uuidgen`** 是否加入 **`UPROGS`**。 |
| **`@FSCK_PROG@` / `@E2INITRD_PROG@`** | **`fsck` 包装**、**`e2initrd_helper`** 等随平台/打包变化。 |

具体以 **`configure`** 结果为准。

---

## 4. 辅助 / 调试目标（通常不随默认 **`make install`**）

| 目标 | 源文件 | 说明 |
|------|--------|------|
| **`findsuper`** | **`findsuper.c`** | 在设备上扫描 **ext2 magic**；注释写明 **临时工具，不建议作为正式产品依赖**。 |
| **`partinfo`** | **`partinfo.c`** | **Linux hd  ioctl** 打印分区/磁盘信息（**`com_err`**）。 |
| **`e2initrd_helper`** | **`e2initrd_helper.c`** | 与 **initrd 中的 blkid/设备枚举** 协作（**`LPROGS`**）。 |
| **`base_device`**（测试） | **`base_device.c` 带 `-DDEBUG`** | **`fullcheck`** 与 **`base_device.tst`** 对照。 |

---

## 5. 头文件与脚本

| 文件 | 作用 |
|------|------|
| **`mke2fs.h`、`tune2fs.h`、`create_inode.h`、`fsck.h`、`e2freefrag.h`、`fsmap.h`** | 对应模块的宏与声明。 |
| **`e2fuzz.sh`** | 与 fuzz 相关的 shell 辅助。 |
| **`Android.bp`** | **AOSP** 中对应模块定义。 |

---

## 6. 推荐阅读顺序

1. **`Makefile.in`** 的 **`SPROGS`/`UPROGS`** 与各 **`: foo.o ... $(LIBS)`** 规则 — 建立「谁链了哪些库」的全局图。  
2. **`mke2fs.c` 主流程** 与 **`mke2fs.conf.5`** — 格式化选项与 profile。  
3. **`tune2fs.c`** — 与 **`journal.c`（来自 debugfs 目录的同一源）** 的配合。  
4. **`fsck.c`** — 与 **`/etc/fstab`、`fsck.*`**  helper 的关系。

---

## 7. 与其它文档的关系

- **e2fsck 多遍检查**：**[e2fsck 代码功能说明](./e2fsck-代码功能说明.md)**  
- **libblkid / libuuid / libe2p**：**[libblkid](./libblkid-代码功能说明.md)**、**[libuuid](./libuuid-代码功能说明.md)**、**[lib/e2p](./libe2p-代码功能说明.md)**  

---

*文档依据 `misc/Makefile.in`、`SRCS`、代表性源文件头注释与链接规则归纳。*
