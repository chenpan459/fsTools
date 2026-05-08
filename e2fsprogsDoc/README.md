# e2fsprogs 1.46.5 源代码文档索引

本目录包含对上游版本 **e2fsprogs 1.46.5**（工作区中为 `fsTools/e2fsprogs-1.46.5`）的阅读型技术说明，面向需要理解 ext2/ext3/ext4 用户态工具与 `libext2fs` 实现关系的开发者。

| 文档 | 说明 |
|------|------|
| [分析报告（主文档）](./e2fsprogs-1.46.5-源码分析.md) | 仓库结构、`libext2fs` 模块划分、I/O 抽象、主要命令行工具与库的对应关系、e2fsck 工作流程概要、与本树 Debian/Ubuntu 打包补丁的简要说明 |
| [**lib/ext2fs 代码功能说明**](./libext2fs-代码功能说明.md) | **专题目录**：`lib/ext2fs` 内各 `.c` 按「打开/关闭、I/O、位图、分配、inode/目录、extent、日志、坏块」等功能分类，Makefile 特殊编入对象说明，推荐阅读顺序 |
| [**lib/uuid 代码功能说明**](./libuuid-代码功能说明.md) | **`libuuid`（DCE UUID）**：`uuid_t`、API 与源文件对应、`pack/unpack`、`gen_uuid` 与 `uuidd`、构建与手册 |
| [**lib/support 代码功能说明**](./libsupport-代码功能说明.md) | **`libsupport`**：`profile`（`mke2fs.conf` 等）、**`plausible`**、**quota**（`quotaio`/`mkquota`/v2/tree）、**`cstring`/`dict`**、`prof_err`、`argv_parse` 与默认 `OBJS` 关系 |
| [**lib/ss 代码功能说明**](./libss-代码功能说明.md) | **`libss`（Subsystem）**：**`.ct` + `mk_cmds` → 命令表**、**`ss_listen`/`execute_cmd`**、**`std_rqs`** 内建请求、与 **readline/com_err** 的关系及 **`debugfs`** 用法概况 |
| [**lib/et 代码功能说明**](./libet-代码功能说明.md) | **`libcom_err`**：**`com_err`/`error_message`**、**`.et` + `compile_et` + awk 生成错误表**、与全树 **`*_err.et`** 的关系 |
| [**lib/e2p 代码功能说明**](./libe2p-代码功能说明.md) | **`libe2p`**：**superblock/日志块可读输出**、**特性/挂载默认/哈希/UUID/OS 字符串互转**、**inode 标志与 project/ioctl**、**`parse_num_*`/`print_flags`** 等工具函数 |
| [**lib/blkid 代码功能说明**](./libblkid-代码功能说明.md) | **`libblkid`**：**块设备内容探测（magic/superblock）**、**TYPE/LABEL/UUID 等 tag 与缓存**、**`blkid.tab` 读写**、与 **`misc/blkid`** 可执行文件及 **`libuuid`** 依赖的关系 |
| [**ext2ed 代码功能说明**](./ext2ed-代码功能说明.md) | **`ext2ed`**：**基于 ncurses 的 ext2 磁盘结构 TUI 编辑器**（描述符驱动、`ext2.descriptors`）、各 **`*_com.c`** 与 **`disk`/`win`** 分工、**默认不编入 e2fsprogs** 及 **约 2GB 上限 / 小端** 等限制 |
| [**e2fsck 代码功能说明**](./e2fsck-代码功能说明.md) | **`e2fsck` 目录**：**`unix.c`/`e2fsck.c` 主流程**、**pass1～5 与 pass1b/1e**、**journal/recovery/revoke**、**`problem`/`message`/`dirinfo`/extent/fscrypt/readahead** 等与 **`Makefile` OBJS** 的对应 |
| [**misc 代码功能说明**](./misc-代码功能说明.md) | **`misc/` 用户态工具**：**`mke2fs`/`tune2fs`/`dumpe2fs`/`badblocks`/`fsck`/`blkid`**、**`e2label`/`e2mmpstatus`/`e2undo`/`fuse2fs`**、**`chattr`/`uuidgen`/`uuidd`** 等与 **`Makefile` 分组、条件编译、共享 `journal`/`util`** 的关系 |

## 源代码位置

```
../e2fsprogs-1.46.5/
```

## 相关上游资料（未打包进本目录）

- `e2fsprogs-1.46.5/INSTALL`、`README`：构建与安装说明  
- `e2fsprogs-1.46.5/doc/`：GNU Info/Texinfo 格式的 `libext2fs` 手册源  
- 各子目录 `.8` / `.5` manual 页：`misc/`、`e2fsck/`、`resize/`、`debugfs/` 等  

生成日期上下文：文档依据当前仓库中的树结构整理，发行说明摘要参考 `debian/changelog` 中 1.46.5-1 条目的上游变更描述。
