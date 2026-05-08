/*
 * Ext4's on-disk acl format.  From linux/fs/ext4/acl.h
 *
 * ---------------------------------------------------------------------------
 * 中文概要
 * ---------------------------------------------------------------------------
 * POSIX ACL（访问控制列表）在用户态常以 xattr 出现：access 存放在扩展属性
 * "system.posix_acl_access"，默认 ACL 存放在 "system.posix_acl_default"。
 * 盘上为 little-endian（__le16/__le32）。头部 a_version（或 ext4 侧专用版本常量）
 * 后接若干条目：需带限定符（用户名/额外组）的条目用带 e_id 的长格式，否则可用
 * 仅 e_tag、e_perm 的短格式条目（如对 owning user/group、mask、other）。
 * 条目序列与语义见 POSIX 1003.1e / Linux ACL 手册 acl(5)。
 * ---------------------------------------------------------------------------
 */

/* 本头文件中 ext4 专用 ACL blob 的版本号（与 xattr POSIX 版本区分开）*/
#define EXT4_ACL_VERSION        0x0001

/* 23.2.5 acl_tag_t values —— 条目类型：所有者/命名用户/组/mask/other 等 */

#define ACL_UNDEFINED_TAG	(0x00)	/* 未使用或占位 */
#define ACL_USER_OBJ		(0x01)	/* 文件所有者（user::）条目 */
#define ACL_USER		(0x02)	/* 具名用户 user:UID: —— 需 e_id */
#define ACL_GROUP_OBJ		(0x04)	/* 文件所属组（group::） */
#define ACL_GROUP		(0x08)	/* 具名组 group:GID: —— 需 e_id */
#define ACL_MASK		(0x10)	/* ACL mask:: 上限条目 */
#define ACL_OTHER		(0x20)	/* other:: 条目 */

/* 23.3.6 acl_type_t values —— 用于区分访问 ACL 与目录默认 ACL 的常量（xattr 命名）*/

#define ACL_TYPE_ACCESS		(0x8000)
#define ACL_TYPE_DEFAULT	(0x4000)

/* 23.2.7 ACL qualifier constants */

#define ACL_UNDEFINED_ID	((id_t)-1)  /* e_id “无此人/占位”惯例 */

/*
 * ACL 条目（长格式）：e_tag ACL_*；e_perm 为 rwx 位（常为读 4、写 2、执行 1 的组合）；
 * e_id 仅对 ACL_USER / ACL_GROUP 有意义，其余通常为 ACL_UNDEFINED_ID 或省略短格式。
 */
typedef struct {
        __le16          e_tag;
        __le16          e_perm;
        __le32          e_id;
 } ext4_acl_entry;
 
/*
 * ACL 条目（短格式）：无 e_id，盘上更短；适用于 USER_OBJ/GROUP_OBJ/MASK/OTHER 等不需限定符的行。
 */
typedef struct {
        __le16          e_tag;
        __le16          e_perm;
} ext4_acl_entry_short;

/*
 * ext4_acl_header：独立于 xattr 包装时的极简头（仅版本）。实际 xattr 中常用下方
 * posix_acl_xattr_header（与 Linux VFS posix_acl 一致）。
 */
typedef struct {
         __le32          a_version;
} ext4_acl_header;


/* Supported ACL a_version fields —— Linux xattr 中 POSIX ACL 列表的版本 */
#define POSIX_ACL_XATTR_VERSION 0x0002

/* 与 posix_acl_* 配套的盘上长条目（与 ext4_acl_entry 布局等价，命名来自内核接口）*/
typedef struct {
        __le16                  e_tag;
        __le16                  e_perm;
        __le32                  e_id;
} posix_acl_xattr_entry;

/*
 * posix_acl_xattr_header：整块 xattr 值布局——版本后跟变长条目数组（柔性数组成员）。
 * a_entries 实际条目数由 xattr 总长度推导：xattr_size = sizeof(header) + n * sizeof(entry)。
 */
typedef struct {
        __le32                  a_version;
#if __GNUC_PREREQ (4, 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
        posix_acl_xattr_entry   a_entries[0];
#if __GNUC_PREREQ (4, 8)
#pragma GCC diagnostic pop
#endif
} posix_acl_xattr_header;
