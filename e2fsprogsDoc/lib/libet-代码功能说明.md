# lib/et 代码功能说明（e2fsprogs 1.46.5）

本文归纳 **`e2fsprogs-1.46.5/lib/et/`**：**`com_err`** 公共错误报告库（**`libcom_err`**），以及从 **`.et`**（error table）源生成 **`.c` / `.h`** 的工具 **`compile_et`**（MIT 系分布式软件常用模式，Kerberos 同源）。

**路径**：`../e2fsprogs-1.46.5/lib/et/`

---

## 1. 工作流程（概念）

1. 开发者维护 **`xxx.et`**：列出错误码基值、消息串、子码等（语法见 **`compile_et.1`**、`et_c.awk`/`et_h.awk` 期望的格式）。  
2. 运行 **`compile_et xxx.et`**（实际是 **`compile_et.sh.in`** 经配置生成的 shell）：调用 **`awk -f et_h.awk`** 生成 **`xxx.h`**，**`awk -f et_c.awk`** 生成 **`xxx.c`**（含 **`error_message`** 表与 **`initialize_error_table`** / **`struct error_table`** 等）。  
3. 程序在启动时 **`add_error_table`** / **`init_error_table`** 注册表；出错时 **`com_err(progname, errcode, fmt, ...)`** 或 **`error_message(code)`** 取可读字符串。  
4. 多语言：`set_com_err_gettext` 可将 **`error_message`** 走 **gettext**（与 **`misc`** 里 **`setlocale`** 配合）。

整个 e2fsprogs 中大量 **`*_err.et`**（如 **`ext2_err.et`、`prof_err.et`**）均依赖此链路。

---

## 2. `libcom_err` 的组成（`Makefile.in` 中 `OBJS`）

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`error_message.o`** | **`error_message.c`** | **`error_message(long)`**：按已注册表将数值码映射为 **C 字符串消息**。 |
| **`et_name.o`** | **`et_name.c`** | 错误表名 / 解析辅助（与内部 **`error_table`** 管理相关）。 |
| **`init_et.o`** | **`init_et.c`** | 库初始化、表链表 **`et_list`** 基元。 |
| **`com_err.o`** | **`com_err.c`** | **`com_err` / `com_err_va`**：向 **stderr** 打印 `prog: error_message(code) (用户附加 fmt)`；钩子 **`com_err_hook`**。 |
| **`com_right.o`** | **`com_right.c`** | **`com_right` / `com_right_r` / `initialize_error_table_r`** 等：**Heimdall** 等兼容 API；与 **`et_list`** 可重入风格表一起使用。 |

**注意**：目录中存在 **`vfprintf.c`** 等文件时，若**未**出现在当前 **`OBJS`** 中，则可能仅供特定平台或历史对接，**默认 `libcom_err` 链接集以 Makefile 为准**。

---

## 3. 对外头文件与类型

| 文件 | 作用 |
|------|------|
| **`com_err.h`** | **`errcode_t`**（通常为 **`long`**）、**`struct error_table`**、**`com_err`、`error_message`、`add_error_table`、`remove_error_table`、`set_com_err_gettext`**、线程相关 **`et_list_lock/unlock`**（若启用）等。 |
| **`error_table.h`** | 部分生成的 **`*.h`** 会包含的宏/助手（与 **`compile_et`** 输出配合；读具体 **`*.et`** 产物可知）。 |
| **`internal.h`** | **`libcom_err` 内部**实现共享，应用程序勿直接包含。 |

---

## 4. `compile_et` 与 awk 模板

| 文件 | 作用 |
|------|------|
| **`compile_et.sh.in`** | 配置后安装为 **`compile_et`**：**`AWK`** 执行 **`et_h.awk`、`et_c.awk`**；支持 **`_ET_DIR_OVERRIDE`** 指向源码树中的模板（构建 **ext2_err** 等时用）。 |
| **`et_h.awk` / `et_c.awk`** | 从 **`.et`** 生成头文件与 C 源。 |
| **`compile_et.1`** | 手册： **`compile_et foo`** → **`foo.h`、`foo.c`**。 |

---

## 5. 文档与测试

| 路径 | 说明 |
|------|------|
| **`com_err.texinfo` / `com_err.3`** | **`com_err` 库文档**（Texinfo / man）。 |
| **`test_cases/*.et`** | 简易/续行/Heimdall 兼容等样例；对应 **`*.c`/`*.h`** 用于回归。 |

---

## 6. 在 e2fsprogs 中的上下游

- **上游**：各子目录 **`*.et`** → **`compile_et`** → **`ext2_err.c`、`ss_err.c`、`prof_err.c`** 等。  
- **下游**：几乎所有命令行工具链接 **`-lcom_err`**，与 **`libext2fs`**、**`libss`** 等一起使用 **`com_err()`** 输出一致风格错误。

---

## 7. 推荐阅读顺序

1. **`com_err.h`** — API 全貌。  
2. **`compile_et.1`** + 任选一个 **`test_cases/simple.et`** 生成结果 — 理解 **`.et` → C** 的形态。  
3. **`com_err.c` + `error_message.c`** — 控制台输出与查表路径。

---

## 8. 与其它文档的关系

总览见 **[e2fsprogs-1.46.5-源码分析.md](./e2fsprogs-1.46.5-源码分析.md)**；**`lib/ss`**、**`lib/support`** 等文档中的 **`compile_et`/`com_err`** 表述可与本文交叉查阅。

---

*文档依据 `lib/et/Makefile.in` 的 `OBJS`/`SRCS` 与 `com_err.h`、`compile_et.sh.in` 归纳。*
