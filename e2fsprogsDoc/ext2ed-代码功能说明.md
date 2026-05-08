# ext2ed 代码功能说明（e2fsprogs 1.46.5）

本文归纳 **`e2fsprogs-1.46.5/ext2ed/`**：**ext2ed（Extended-2 Filesystem Editor）**——基于 **ncurses** 的 **ext2 磁盘结构交互式查看/编辑**程序。原始作者为 **Gadi Oxman（1995）**；e2fsprogs  tree 中的版本经 **Theodore Ts’o** 维护以适配现代 **ncurses** 等。

**路径**：`../e2fsprogs-1.46.5/ext2ed/`

---

## 1. 构建与定位（必读）

| 项目 | 说明 |
|------|------|
| **默认是否编入 e2fsprogs** | **顶层 Makefile 通常不进入本目录**，即**默认不编译 `ext2ed`**（有意为之）。 |
| **链接库** | **`Makefile.in`**：`LIBS = -lncurses $(LIBEXT2FS)`，使用 **`libext2fs`** 中的 **`ext2_fs.h` 等**常量与布局。 |
| **上游态度（`README`）** | **Ts’o 明确不建议发行版打包**：程序存在 **严重局限**（见下），收录在源码树中主要便于开发者在 **Intel、小镜像** 上**选择性损坏文件系统以构造测试用例**，**不提供一般用户支持**。 |

### 1.1 已知严重局限（源码与自述一致）

1. **容量**： **`disk.c`** 等大量使用 **`fseek`/`long` 偏移**，实践中 **无法可靠处理大于约 2GB 的文件系统**（注释亦提到若改为 **`llseek`** 等才能面向更大卷）。  
2. **字节序**： 假设 **little-endian（Intel）**，未做通用-endian 适配。  
3. **交互**： 历史上曾同时用 **readline + ncurses**；README 写明二者冲突，**e2fsprogs 中已禁用 readline 与 ncurses 的混用**，避免刷屏与重绘问题。

---

## 2. 程序结构总览

整体模式：**「类型描述符 + 命令表 + 分发器」**。  

- **`struct struct_descriptor`**：从 **`ext2.descriptors`**（及可选备用描述文件）解析出的 **“对象类型”**（字段名、类型、布局），形成链表 **`first_type` … `last_type`**。  
- **`struct struct_commands`**：每类对象挂一组 **命令名 → 回调 `PF`**（如 inode、目录、位图各有子命令）。  
- **`main.c`**：**`init()`** → **`parser()`** 循环读命令 → **`dispatch()`** 解析并调用对应 **`type_*___*`** 或通用命令。

终端 UI（**`win.c`**）：**标题窗 / 状态窗 / 大 **`show_pad`** / 命令窗**，支持 **SIGWINCH** 重绘（**`init_signals`**）。

---

## 3. 源文件与职责（`Makefile.in` 中 `OBJS`）

| 对象文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`main.o`** | **`main.c`** | **`main`**：**`-w` 写权限**、`set_device` 打开设备；**`parser`/`dispatch`**；**命令补全**（若编译 **readline**）；全局状态与配置默认值。 |
| **`init.o`** | **`init.c`** | 读 **配置文件**（**`ext2ed.conf.in` → `ext2ed.conf`**）、装载 **结构描述**、注册 **通用命令** 与 **ext2 通用命令**、**信号处理**、**readline 初始化**。 |
| **`general_com.o`** | **`general_com.c`** | 始终可用的命令：**`help`、`setdevice`、`show`、`pgup`/`pgdn`、`cd`、`remember`/`recall`、写开关 **`enable_write`/`disable_write`/`write_data`** 等。 |
| **`ext2_com.o`** | **`ext2_com.c`** | 已进入 ext2 模式后的顶层导航：**`super`、`group`、根路径 **`cd`** 等**（见 **`ext2ed.h` 声明）。 |
| **`super_com.o`** | **`super_com.c`** | **`ext2_super_block`**：显示、多备份 **`gocopy`**、**`setactivecopy`**。 |
| **`group_com.o`** | **`group_com.c`** | **`ext2_group_desc`**：组间切换、进入 **inode 表/块位图/inode 位图** 等。 |
| **`inode_com.o`** | **`inode_com.c`** | **`ext2_inode`**：相邻 inode、进入 **文件/目录** 视图等。 |
| **`file_com.o`** | **`file_com.c`** | **“文件”对象**：按块遍历、十六进制/文本显示、**间接块**解析（单/双/三间接）、**`writedata`**。 |
| **`dir_com.o`** | **`dir_com.c`** | **目录**：目录项列表、**`followinode`**、分页、**`cd`** 等；内部用回调扫描 **`ext2_dir_entry`**。 |
| **`blockbitmap_com.o`** | **`blockbitmap_com.c`** | **块位图**：条目跳转、**分配/释放位**（配合写权限）。 |
| **`inodebitmap_com.o`** | **`inodebitmap_com.c`** | **inode 位图**：同上。 |
| **`disk.o`** | **`disk.c`** | 所有读写经 **`low_read`/`low_write`**；**`fseek`**；可选 **`log_changes`** 把写入记日志。 |
| **`win.o`** | **`win.c`** | **ncurses** 窗口创建与刷新。 |

---

## 4. 配置与数据文件

| 文件 | 作用 |
|------|------|
| **`ext2ed.conf.in`** | 安装为 **`ext2ed.conf`**：**允许编辑/只读挂载读/强制 ext2/日志路径** 等（详见模板内注释）。 |
| **`ext2.descriptors`** | **结构定义源**：语法为简化 **“struct … { … }”**，由 **`init.c`** 解析生成可编辑的 **字段元数据**（与内核 **`ext2_fs.h`** 同源思路，见文件头说明）。用户可通过 **`AlternateDescriptors`** 追加类型。 |
| **`doc/*.sgml`** | **用户指南、ext2 概述、设计与实现**；**`Makefile.in`** 提供 **`.sgml` → `.ps`/`.pdf`/`.html`**（依赖 **`sgmltools`**，通常开发者手工生成文档）。 |
| **`ext2ed.8.in`** | **`man 8`** 手册页源。 |

---

## 5. 与 `debugfs` 的关系

二者都能检查 ext 系文件系统，但 **ext2ed** 侧重点是 **“按 on-disk 结构类型分层的 TUI 编辑器 + 描述符驱动”**，且 **局限更大、默认不构建**；日常维护与脚本更常用 **`debugfs`**。阅读本目录时建议对照 **`debugfs/`** 与 **`lib/ext2fs`**。

---

## 6. 推荐阅读顺序

1. **`README`**（尤其 **Ts’o** 与原作者章节）— 期望与禁忌。  
2. **`ext2ed.h`** — 全局结构与命令注册表。  
3. **`init.c`**（**`set_struct_descriptors`、`add_user_command`**）— 类型系统如何挂上命令。  
4. **`disk.c` + `file_com.c`**（间接块）— 理解 **偏移与 2G 瓶颈** 的来源。

---

## 7. 与其它文档的关系

通用的 ext2/ext3/ext4 **on-disk 布局**仍以 **[libext2fs 代码功能说明](./libext2fs-代码功能说明.md)** 与 **`lib/ext2fs/ext2_fs.h`** 为准；本文仅描述 **ext2ed 子目录**内的交互与代码组织。

---

*文档依据 `ext2ed/Makefile.in`、`ext2ed.h`、`README`、`disk.c` 与 `init.c` 归纳。*
