/*
 * Copyright (c) 2003,2004 Cluster File Systems, Inc, info@clusterfs.com
 * Written by Alex Tomas <alex@clusterfs.com>
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Library
 * General Public License, version 2.
 * %End-Header%
 */

#ifndef _LINUX_EXT3_EXTENTS
#define _LINUX_EXT3_EXTENTS

/*
 * ---------------------------------------------------------------------------
 * ext4 extent 映射（盘上 B+ 树概要，中文）
 * ---------------------------------------------------------------------------
 * 启用 extent 特性时 inode 的 i_block[]（60 字节）存放树根：
 *   eh_depth==0：条目为 ext3_extent（叶子）；eh_depth>0：条目为 ext3_extent_idx
 *   （索引），ei_leaf 指向下一层磁盘块。深层结点各占一整块，块开头同样是 header。
 * metadata_csum 时块尾可含 ext3_extent_tail；extent 与 index 各 12 字节便于对齐。
 * 命名 ext3_* 为历史兼容，与内核 ext4 extent 布局一致。
 * ---------------------------------------------------------------------------
 */

/*
 * ext3_inode has i_block array (total 60 bytes)
 * first 4 bytes are used to store:
 *  - tree depth (0 mean there is no tree yet. all extents in the inode)
 *  - number of alive extents in the inode
 *
 * 注：具体以盘上 ext3_extent_header.eh_depth、eh_entries 为准；inode 内联根与外置
 * extent 块格式一致。
 */

/*
 * This is extent tail on-disk structure.
 * All other extent structures are 12 bytes long.  It turns out that
 * block_size % 12 >= 4 for at least all powers of 2 greater than 512, which
 * covers all valid ext4 block sizes.  Therefore, this tail structure can be
 * crammed into the end of the block without having to rebalance the tree.
 *
 * 块尾 4 字节校验（metadata_csum）：crc32c(超级块种子等 + inode + 块号)。
 */
struct ext3_extent_tail {
	__le32	et_checksum;	/* crc32c(uuid+inum+extent_block) */
};

/*
 * this is extent on-disk structure
 * it's used at the bottom of the tree
 *
 * 叶子：从逻辑块 ee_block 起连续若干 fs 块，映射到物理 48 位起点（ee_start_hi:ee_start）。
 */
struct ext3_extent {
	__le32	ee_block;	/* first logical block extent covers；区间内首逻辑块号 */
	__le16	ee_len;		/* number of blocks + 初始化位，见下文 EXT_*_MAX_LEN */
	__le16	ee_start_hi;	/* high 16 bits of physical block */
	__le32	ee_start;	/* low 32 bits of physical block（原版注释 typo: bigs）*/
};

/*
 * this is index on-disk structure
 * it's used at all the levels, but the bottom
 *
 * 索引：ei_block 为子键起点；ei_leaf_hi:ei_leaf 为下一层 extent 结点块号。
 */
struct ext3_extent_idx {
	__le32	ei_block;	/* index covers logical blocks from 'block'；子树键下限 */
	__le32	ei_leaf;	/* pointer to the physical block of the next *
				 * level. leaf or next index could bet here */
	__le16	ei_leaf_hi;	/* high 16 bits of physical block */
	__le16	ei_unused;	/* padding / 预留 */
};

/*
 * each block (leaves and indexes), even inode-stored has header
 *
 * extent 结点块（含 inode 内 60 字节根）必须以本结构开头；后接 eh_entries 条
 * extent 或 extent_idx，由 eh_depth 区分叶与索引。
 */
struct ext3_extent_header {
	__le16	eh_magic;	/* probably will support different formats；须等于 EXT3_EXT_MAGIC */
	__le16	eh_entries;	/* number of valid entries；当前已用槽位数 */
	__le16	eh_max;		/* capacity；本结点至多可容纳条目（与块大小、tail 有关）*/
	__le16	eh_depth;	/* 0=叶子(extent)，>0 为索引(idx) */
	__le32	eh_generation;	/* generation of the tree；兼容保留，常为 0 */
};

/* extent 结点魔数常量（盘上 eh_magic 为小端与其它字段一同存储）*/
#define EXT3_EXT_MAGIC		0xf30a

/*
 * array of ext3_ext_path contains path to some extent
 * creation/lookup routines use it for traversal/splitting/etc
 * truncate uses it to simulate recursive walking
 *
 * 运行时从根到叶的路径栈（非盘上结构）；p_hdr/p_ext/p_idx 指向当前层解析结果。
 */
struct ext3_ext_path {
	__u32				p_block;  /* 本层结点物理块号，根常为 0 */
	__u16				p_depth;  /* 该层深度 */
	struct ext3_extent		*p_ext;
	struct ext3_extent_idx		*p_idx;
	struct ext3_extent_header	*p_hdr;
	struct buffer_head		*p_bh; /* lib 中为缓冲控制块，类比内核 bh */
};

/*
 * EXT_INIT_MAX_LEN is the maximum number of blocks we can have in an
 * initialized extent. This is 2^15 and not (2^16 - 1), since we use the
 * MSB of ee_len field in the extent datastructure to signify if this
 * particular extent is an initialized extent or an uninitialized (i.e.
 * preallocated).
 * EXT_UNINIT_MAX_LEN is the maximum number of blocks we can have in an
 * uninitialized extent.
 * If ee_len is <= 0x8000, it is an initialized extent. Otherwise, it is an
 * uninitialized one. In other words, if MSB of ee_len is set, it is an
 * uninitialized extent with only one special scenario when ee_len = 0x8000.
 * In this case we can not have an uninitialized extent of zero length and
 * thus we make it as a special case of initialized extent with 0x8000 length.
 * This way we get better extent-to-group alignment for initialized extents.
 * Hence, the maximum number of blocks we can have in an *initialized*
 * extent is 2^15 (32768) and in an *uninitialized* extent is 2^15-1 (32767).
 *
 * 中文摘要：ee_len 最高位标记未初始化(预分配) extent；特例 0x8000 表示已初始化长度
 * 恰为 32768。逻辑块号、物理块号协议上限见 EXT_MAX_EXTENT_*。
 */
#define EXT_INIT_MAX_LEN	(1UL << 15)
#define EXT_UNINIT_MAX_LEN	(EXT_INIT_MAX_LEN - 1)
#define EXT_MAX_EXTENT_LBLK	(((__u64) 1 << 32) - 1)
#define EXT_MAX_EXTENT_PBLK	(((__u64) 1 << 48) - 1)

/* 由 header 得到首条 extent / idx；LAST 指最后一条有效项，MAX 指按 eh_max 的末槽（边界）
 */
#define EXT_FIRST_EXTENT(__hdr__) \
	((struct ext3_extent *) (((char *) (__hdr__)) +		\
				 sizeof(struct ext3_extent_header)))
#define EXT_FIRST_INDEX(__hdr__) \
	((struct ext3_extent_idx *) (((char *) (__hdr__)) +	\
				     sizeof(struct ext3_extent_header)))
#define EXT_HAS_FREE_INDEX(__path__) \
	(ext2fs_le16_to_cpu((__path__)->p_hdr->eh_entries) < \
	 ext2fs_le16_to_cpu((__path__)->p_hdr->eh_max))
#define EXT_LAST_EXTENT(__hdr__) \
	(EXT_FIRST_EXTENT((__hdr__)) + \
	ext2fs_le16_to_cpu((__hdr__)->eh_entries) - 1)
#define EXT_LAST_INDEX(__hdr__) \
	(EXT_FIRST_INDEX((__hdr__)) + \
	ext2fs_le16_to_cpu((__hdr__)->eh_entries) - 1)
#define EXT_MAX_EXTENT(__hdr__) \
	(EXT_FIRST_EXTENT((__hdr__)) + \
	ext2fs_le16_to_cpu((__hdr__)->eh_max) - 1)
#define EXT_MAX_INDEX(__hdr__) \
	(EXT_FIRST_INDEX((__hdr__)) + \
	ext2fs_le16_to_cpu((__hdr__)->eh_max) - 1)

#endif /* _LINUX_EXT3_EXTENTS */

