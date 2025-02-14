/* SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0 */
/* Copyright (c) 2015-2018 Mellanox Technologies. All rights reserved */

#ifndef _MLXSW_SPECTRUM_H
#define _MLXSW_SPECTRUM_H

#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/rhashtable.h>
#include <linux/bitops.h>
#include <linux/if_vlan.h>
#include <linux/list.h>
#include <linux/dcbnl.h>
#include <linux/in6.h>
#include <linux/notifier.h>
#include <net/psample.h>
#include <net/pkt_cls.h>
#include <net/red.h>

#include "port.h"
#include "core.h"
#include "core_acl_flex_keys.h"
#include "core_acl_flex_actions.h"
#include "reg.h"

#define MLXSW_SP_FID_8021D_MAX 1024

#define MLXSW_SP_MID_MAX 7000

#define MLXSW_SP_PORTS_PER_CLUSTER_MAX 4

#define MLXSW_SP_PORT_BASE_SPEED 25000	/* Mb/s */

#define MLXSW_SP_KVD_LINEAR_SIZE 98304 /* entries */
#define MLXSW_SP_KVD_GRANULARITY 128

#define MLXSW_SP_RESOURCE_NAME_KVD "kvd"
#define MLXSW_SP_RESOURCE_NAME_KVD_LINEAR "linear"
#define MLXSW_SP_RESOURCE_NAME_KVD_HASH_SINGLE "hash_single"
#define MLXSW_SP_RESOURCE_NAME_KVD_HASH_DOUBLE "hash_double"
#define MLXSW_SP_RESOURCE_NAME_KVD_LINEAR_SINGLES "singles"
#define MLXSW_SP_RESOURCE_NAME_KVD_LINEAR_CHUNKS "chunks"
#define MLXSW_SP_RESOURCE_NAME_KVD_LINEAR_LARGE_CHUNKS "large_chunks"

enum mlxsw_sp_resource_id {
	MLXSW_SP_RESOURCE_KVD = 1,
	MLXSW_SP_RESOURCE_KVD_LINEAR,
	MLXSW_SP_RESOURCE_KVD_HASH_SINGLE,
	MLXSW_SP_RESOURCE_KVD_HASH_DOUBLE,
	MLXSW_SP_RESOURCE_KVD_LINEAR_SINGLE,
	MLXSW_SP_RESOURCE_KVD_LINEAR_CHUNKS,
	MLXSW_SP_RESOURCE_KVD_LINEAR_LARGE_CHUNKS,
};

struct mlxsw_sp_port;
struct mlxsw_sp_rif;
struct mlxsw_sp_span_entry;

struct mlxsw_sp_upper {
	struct net_device *dev;
	unsigned int ref_count;
};

enum mlxsw_sp_rif_type {
	MLXSW_SP_RIF_TYPE_SUBPORT,
	MLXSW_SP_RIF_TYPE_VLAN,
	MLXSW_SP_RIF_TYPE_FID,
	MLXSW_SP_RIF_TYPE_IPIP_LB, /* IP-in-IP loopback. */
	MLXSW_SP_RIF_TYPE_MAX,
};

enum mlxsw_sp_fid_type {
	MLXSW_SP_FID_TYPE_8021Q,
	MLXSW_SP_FID_TYPE_8021D,
	MLXSW_SP_FID_TYPE_RFID,
	MLXSW_SP_FID_TYPE_DUMMY,
	MLXSW_SP_FID_TYPE_MAX,
};

struct mlxsw_sp_mid {
	struct list_head list;
	unsigned char addr[ETH_ALEN];
	u16 fid;
	u16 mid;
	bool in_hw;
	unsigned long *ports_in_mid; /* bits array */
};

enum mlxsw_sp_port_mall_action_type {
	MLXSW_SP_PORT_MALL_MIRROR,
	MLXSW_SP_PORT_MALL_SAMPLE,
};

struct mlxsw_sp_port_mall_mirror_tc_entry {
	int span_id;
	bool ingress;
};

struct mlxsw_sp_port_mall_tc_entry {
	struct list_head list;
	unsigned long cookie;
	enum mlxsw_sp_port_mall_action_type type;
	union {
		struct mlxsw_sp_port_mall_mirror_tc_entry mirror;
	};
};

struct mlxsw_sp_sb;
struct mlxsw_sp_bridge;
struct mlxsw_sp_router;
struct mlxsw_sp_mr;
struct mlxsw_sp_acl;
struct mlxsw_sp_counter_pool;
struct mlxsw_sp_fid_core;
struct mlxsw_sp_kvdl;
struct mlxsw_sp_kvdl_ops;
struct mlxsw_sp_mr_tcam_ops;
struct mlxsw_sp_acl_tcam_ops;

struct mlxsw_sp {
	struct mlxsw_sp_port **ports;
	struct mlxsw_core *core;
	const struct mlxsw_bus_info *bus_info;
	unsigned char base_mac[ETH_ALEN];
	struct mlxsw_sp_upper *lags;
	int *port_to_module;
	struct mlxsw_sp_sb *sb;
	struct mlxsw_sp_bridge *bridge;
	struct mlxsw_sp_router *router;
	struct mlxsw_sp_mr *mr;
	struct mlxsw_afa *afa;
	struct mlxsw_sp_acl *acl;
	struct mlxsw_sp_fid_core *fid_core;
	struct mlxsw_sp_kvdl *kvdl;
	struct notifier_block netdevice_nb;

	struct mlxsw_sp_counter_pool *counter_pool;
	struct {
		struct mlxsw_sp_span_entry *entries;
		int entries_count;
	} span;
	const struct mlxsw_fw_rev *req_rev;
	const char *fw_filename;
	const struct mlxsw_sp_kvdl_ops *kvdl_ops;
	const struct mlxsw_afa_ops *afa_ops;
	const struct mlxsw_afk_ops *afk_ops;
	const struct mlxsw_sp_mr_tcam_ops *mr_tcam_ops;
	const struct mlxsw_sp_acl_tcam_ops *acl_tcam_ops;
};

static inline struct mlxsw_sp_upper *
mlxsw_sp_lag_get(struct mlxsw_sp *mlxsw_sp, u16 lag_id)
{
	return &mlxsw_sp->lags[lag_id];
}

struct mlxsw_sp_port_pcpu_stats {
	u64			rx_packets;
	u64			rx_bytes;
	u64			tx_packets;
	u64			tx_bytes;
	struct u64_stats_sync	syncp;
	u32			tx_dropped;
};

struct mlxsw_sp_port_sample {
	struct psample_group __rcu *psample_group;
	u32 trunc_size;
	u32 rate;
	bool truncate;
};

struct mlxsw_sp_bridge_port;
struct mlxsw_sp_fid;

struct mlxsw_sp_port_vlan {
	struct list_head list;
	struct mlxsw_sp_port *mlxsw_sp_port;
	struct mlxsw_sp_fid *fid;
	unsigned int ref_count;
	u16 vid;
	struct mlxsw_sp_bridge_port *bridge_port;
	struct list_head bridge_vlan_node;
};

/* No need an internal lock; At worse - miss a single periodic iteration */
struct mlxsw_sp_port_xstats {
	u64 ecn;
	u64 wred_drop[TC_MAX_QUEUE];
	u64 tail_drop[TC_MAX_QUEUE];
	u64 backlog[TC_MAX_QUEUE];
	u64 tx_bytes[IEEE_8021QAZ_MAX_TCS];
	u64 tx_packets[IEEE_8021QAZ_MAX_TCS];
};

struct mlxsw_sp_port {
	struct net_device *dev;
	struct mlxsw_sp_port_pcpu_stats __percpu *pcpu_stats;
	struct mlxsw_sp *mlxsw_sp;
	u8 local_port;
	u8 lagged:1,
	   split:1;
	u16 pvid;
	u16 lag_id;
	struct {
		u8 tx_pause:1,
		   rx_pause:1,
		   autoneg:1;
	} link;
	struct {
		struct ieee_ets *ets;
		struct ieee_maxrate *maxrate;
		struct ieee_pfc *pfc;
		enum mlxsw_reg_qpts_trust_state trust_state;
	} dcb;
	struct {
		u8 module;
		u8 width;
		u8 lane;
	} mapping;
	/* TC handles */
	struct list_head mall_tc_list;
	struct {
		#define MLXSW_HW_STATS_UPDATE_TIME HZ
		struct rtnl_link_stats64 stats;
		struct mlxsw_sp_port_xstats xstats;
		struct delayed_work update_dw;
	} periodic_hw_stats;
	struct mlxsw_sp_port_sample *sample;
	struct list_head vlans_list;
	struct mlxsw_sp_qdisc *root_qdisc;
	struct mlxsw_sp_qdisc *tclass_qdiscs;
	unsigned acl_rule_count;
	struct mlxsw_sp_acl_block *ing_acl_block;
	struct mlxsw_sp_acl_block *eg_acl_block;
};

static inline bool
mlxsw_sp_port_is_pause_en(const struct mlxsw_sp_port *mlxsw_sp_port)
{
	return mlxsw_sp_port->link.tx_pause || mlxsw_sp_port->link.rx_pause;
}

static inline struct mlxsw_sp_port *
mlxsw_sp_port_lagged_get(struct mlxsw_sp *mlxsw_sp, u16 lag_id, u8 port_index)
{
	struct mlxsw_sp_port *mlxsw_sp_port;
	u8 local_port;

	local_port = mlxsw_core_lag_mapping_get(mlxsw_sp->core,
						lag_id, port_index);
	mlxsw_sp_port = mlxsw_sp->ports[local_port];
	return mlxsw_sp_port && mlxsw_sp_port->lagged ? mlxsw_sp_port : NULL;
}

static inline struct mlxsw_sp_port_vlan *
mlxsw_sp_port_vlan_find_by_vid(const struct mlxsw_sp_port *mlxsw_sp_port,
			       u16 vid)
{
	struct mlxsw_sp_port_vlan *mlxsw_sp_port_vlan;

	list_for_each_entry(mlxsw_sp_port_vlan, &mlxsw_sp_port->vlans_list,
			    list) {
		if (mlxsw_sp_port_vlan->vid == vid)
			return mlxsw_sp_port_vlan;
	}

	return NULL;
}

enum mlxsw_sp_flood_type {
	MLXSW_SP_FLOOD_TYPE_UC,
	MLXSW_SP_FLOOD_TYPE_BC,
	MLXSW_SP_FLOOD_TYPE_MC,
};

/* spectrum_buffers.c */
int mlxsw_sp_buffers_init(struct mlxsw_sp *mlxsw_sp);
void mlxsw_sp_buffers_fini(struct mlxsw_sp *mlxsw_sp);
int mlxsw_sp_port_buffers_init(struct mlxsw_sp_port *mlxsw_sp_port);
int mlxsw_sp_sb_pool_get(struct mlxsw_core *mlxsw_core,
			 unsigned int sb_index, u16 pool_index,
			 struct devlink_sb_pool_info *pool_info);
int mlxsw_sp_sb_pool_set(struct mlxsw_core *mlxsw_core,
			 unsigned int sb_index, u16 pool_index, u32 size,
			 enum devlink_sb_threshold_type threshold_type);
int mlxsw_sp_sb_port_pool_get(struct mlxsw_core_port *mlxsw_core_port,
			      unsigned int sb_index, u16 pool_index,
			      u32 *p_threshold);
int mlxsw_sp_sb_port_pool_set(struct mlxsw_core_port *mlxsw_core_port,
			      unsigned int sb_index, u16 pool_index,
			      u32 threshold);
int mlxsw_sp_sb_tc_pool_bind_get(struct mlxsw_core_port *mlxsw_core_port,
				 unsigned int sb_index, u16 tc_index,
				 enum devlink_sb_pool_type pool_type,
				 u16 *p_pool_index, u32 *p_threshold);
int mlxsw_sp_sb_tc_pool_bind_set(struct mlxsw_core_port *mlxsw_core_port,
				 unsigned int sb_index, u16 tc_index,
				 enum devlink_sb_pool_type pool_type,
				 u16 pool_index, u32 threshold);
int mlxsw_sp_sb_occ_snapshot(struct mlxsw_core *mlxsw_core,
			     unsigned int sb_index);
int mlxsw_sp_sb_occ_max_clear(struct mlxsw_core *mlxsw_core,
			      unsigned int sb_index);
int mlxsw_sp_sb_occ_port_pool_get(struct mlxsw_core_port *mlxsw_core_port,
				  unsigned int sb_index, u16 pool_index,
				  u32 *p_cur, u32 *p_max);
int mlxsw_sp_sb_occ_tc_port_bind_get(struct mlxsw_core_port *mlxsw_core_port,
				     unsigned int sb_index, u16 tc_index,
				     enum devlink_sb_pool_type pool_type,
				     u32 *p_cur, u32 *p_max);
u32 mlxsw_sp_cells_bytes(const struct mlxsw_sp *mlxsw_sp, u32 cells);
u32 mlxsw_sp_bytes_cells(const struct mlxsw_sp *mlxsw_sp, u32 bytes);

/* spectrum_switchdev.c */
int mlxsw_sp_switchdev_init(struct mlxsw_sp *mlxsw_sp);
void mlxsw_sp_switchdev_fini(struct mlxsw_sp *mlxsw_sp);
void mlxsw_sp_port_switchdev_init(struct mlxsw_sp_port *mlxsw_sp_port);
void mlxsw_sp_port_switchdev_fini(struct mlxsw_sp_port *mlxsw_sp_port);
int mlxsw_sp_rif_fdb_op(struct mlxsw_sp *mlxsw_sp, const char *mac, u16 fid,
			bool adding);
void
mlxsw_sp_port_vlan_bridge_leave(struct mlxsw_sp_port_vlan *mlxsw_sp_port_vlan);
int mlxsw_sp_port_bridge_join(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct net_device *brport_dev,
			      struct net_device *br_dev,
			      struct netlink_ext_ack *extack);
void mlxsw_sp_port_bridge_leave(struct mlxsw_sp_port *mlxsw_sp_port,
				struct net_device *brport_dev,
				struct net_device *br_dev);
bool mlxsw_sp_bridge_device_is_offloaded(const struct mlxsw_sp *mlxsw_sp,
					 const struct net_device *br_dev);

/* spectrum.c */
int mlxsw_sp_port_ets_set(struct mlxsw_sp_port *mlxsw_sp_port,
			  enum mlxsw_reg_qeec_hr hr, u8 index, u8 next_index,
			  bool dwrr, u8 dwrr_weight);
int mlxsw_sp_port_prio_tc_set(struct mlxsw_sp_port *mlxsw_sp_port,
			      u8 switch_prio, u8 tclass);
int __mlxsw_sp_port_headroom_set(struct mlxsw_sp_port *mlxsw_sp_port, int mtu,
				 u8 *prio_tc, bool pause_en,
				 struct ieee_pfc *my_pfc);
int mlxsw_sp_port_ets_maxrate_set(struct mlxsw_sp_port *mlxsw_sp_port,
				  enum mlxsw_reg_qeec_hr hr, u8 index,
				  u8 next_index, u32 maxrate);
enum mlxsw_reg_spms_state mlxsw_sp_stp_spms_state(u8 stp_state);
int mlxsw_sp_port_vid_stp_set(struct mlxsw_sp_port *mlxsw_sp_port, u16 vid,
			      u8 state);
int mlxsw_sp_port_vp_mode_set(struct mlxsw_sp_port *mlxsw_sp_port, bool enable);
int mlxsw_sp_port_vid_learning_set(struct mlxsw_sp_port *mlxsw_sp_port, u16 vid,
				   bool learn_enable);
int mlxsw_sp_port_pvid_set(struct mlxsw_sp_port *mlxsw_sp_port, u16 vid);
struct mlxsw_sp_port_vlan *
mlxsw_sp_port_vlan_get(struct mlxsw_sp_port *mlxsw_sp_port, u16 vid);
void mlxsw_sp_port_vlan_put(struct mlxsw_sp_port_vlan *mlxsw_sp_port_vlan);
int mlxsw_sp_port_vlan_set(struct mlxsw_sp_port *mlxsw_sp_port, u16 vid_begin,
			   u16 vid_end, bool is_member, bool untagged);
int mlxsw_sp_flow_counter_get(struct mlxsw_sp *mlxsw_sp,
			      unsigned int counter_index, u64 *packets,
			      u64 *bytes);
int mlxsw_sp_flow_counter_alloc(struct mlxsw_sp *mlxsw_sp,
				unsigned int *p_counter_index);
void mlxsw_sp_flow_counter_free(struct mlxsw_sp *mlxsw_sp,
				unsigned int counter_index);
bool mlxsw_sp_port_dev_check(const struct net_device *dev);
struct mlxsw_sp *mlxsw_sp_lower_get(struct net_device *dev);
struct mlxsw_sp_port *mlxsw_sp_port_dev_lower_find(struct net_device *dev);
struct mlxsw_sp_port *mlxsw_sp_port_lower_dev_hold(struct net_device *dev);
void mlxsw_sp_port_dev_put(struct mlxsw_sp_port *mlxsw_sp_port);
struct mlxsw_sp_port *mlxsw_sp_port_dev_lower_find_rcu(struct net_device *dev);

/* spectrum_dcb.c */
#ifdef CONFIG_MLXSW_SPECTRUM_DCB
int mlxsw_sp_port_dcb_init(struct mlxsw_sp_port *mlxsw_sp_port);
void mlxsw_sp_port_dcb_fini(struct mlxsw_sp_port *mlxsw_sp_port);
#else
static inline int mlxsw_sp_port_dcb_init(struct mlxsw_sp_port *mlxsw_sp_port)
{
	return 0;
}
static inline void mlxsw_sp_port_dcb_fini(struct mlxsw_sp_port *mlxsw_sp_port)
{}
#endif

/* spectrum_router.c */
int mlxsw_sp_router_init(struct mlxsw_sp *mlxsw_sp);
void mlxsw_sp_router_fini(struct mlxsw_sp *mlxsw_sp);
int mlxsw_sp_netdevice_router_port_event(struct net_device *dev);
void mlxsw_sp_rif_macvlan_del(struct mlxsw_sp *mlxsw_sp,
			      const struct net_device *macvlan_dev);
int mlxsw_sp_inetaddr_event(struct notifier_block *unused,
			    unsigned long event, void *ptr);
int mlxsw_sp_inetaddr_valid_event(struct notifier_block *unused,
				  unsigned long event, void *ptr);
int mlxsw_sp_inet6addr_event(struct notifier_block *unused,
			     unsigned long event, void *ptr);
int mlxsw_sp_inet6addr_valid_event(struct notifier_block *unused,
				   unsigned long event, void *ptr);
int mlxsw_sp_netdevice_vrf_event(struct net_device *l3_dev, unsigned long event,
				 struct netdev_notifier_changeupper_info *info);
bool mlxsw_sp_netdev_is_ipip_ol(const struct mlxsw_sp *mlxsw_sp,
				const struct net_device *dev);
bool mlxsw_sp_netdev_is_ipip_ul(const struct mlxsw_sp *mlxsw_sp,
				const struct net_device *dev);
int mlxsw_sp_netdevice_ipip_ol_event(struct mlxsw_sp *mlxsw_sp,
				     struct net_device *l3_dev,
				     unsigned long event,
				     struct netdev_notifier_info *info);
int
mlxsw_sp_netdevice_ipip_ul_event(struct mlxsw_sp *mlxsw_sp,
				 struct net_device *l3_dev,
				 unsigned long event,
				 struct netdev_notifier_info *info);
void
mlxsw_sp_port_vlan_router_leave(struct mlxsw_sp_port_vlan *mlxsw_sp_port_vlan);
void mlxsw_sp_rif_destroy(struct mlxsw_sp_rif *rif);
void mlxsw_sp_rif_destroy_by_dev(struct mlxsw_sp *mlxsw_sp,
				 struct net_device *dev);

/* spectrum_kvdl.c */
enum mlxsw_sp_kvdl_entry_type {
	MLXSW_SP_KVDL_ENTRY_TYPE_ADJ,
	MLXSW_SP_KVDL_ENTRY_TYPE_ACTSET,
	MLXSW_SP_KVDL_ENTRY_TYPE_PBS,
	MLXSW_SP_KVDL_ENTRY_TYPE_MCRIGR,
};

static inline unsigned int
mlxsw_sp_kvdl_entry_size(enum mlxsw_sp_kvdl_entry_type type)
{
	switch (type) {
	case MLXSW_SP_KVDL_ENTRY_TYPE_ADJ: /* fall through */
	case MLXSW_SP_KVDL_ENTRY_TYPE_ACTSET: /* fall through */
	case MLXSW_SP_KVDL_ENTRY_TYPE_PBS: /* fall through */
	case MLXSW_SP_KVDL_ENTRY_TYPE_MCRIGR: /* fall through */
	default:
		return 1;
	}
}

struct mlxsw_sp_kvdl_ops {
	size_t priv_size;
	int (*init)(struct mlxsw_sp *mlxsw_sp, void *priv);
	void (*fini)(struct mlxsw_sp *mlxsw_sp, void *priv);
	int (*alloc)(struct mlxsw_sp *mlxsw_sp, void *priv,
		     enum mlxsw_sp_kvdl_entry_type type,
		     unsigned int entry_count, u32 *p_entry_index);
	void (*free)(struct mlxsw_sp *mlxsw_sp, void *priv,
		     enum mlxsw_sp_kvdl_entry_type type,
		     unsigned int entry_count, int entry_index);
	int (*alloc_size_query)(struct mlxsw_sp *mlxsw_sp, void *priv,
				enum mlxsw_sp_kvdl_entry_type type,
				unsigned int entry_count,
				unsigned int *p_alloc_count);
	int (*resources_register)(struct mlxsw_sp *mlxsw_sp, void *priv);
};

int mlxsw_sp_kvdl_init(struct mlxsw_sp *mlxsw_sp);
void mlxsw_sp_kvdl_fini(struct mlxsw_sp *mlxsw_sp);
int mlxsw_sp_kvdl_alloc(struct mlxsw_sp *mlxsw_sp,
			enum mlxsw_sp_kvdl_entry_type type,
			unsigned int entry_count, u32 *p_entry_index);
void mlxsw_sp_kvdl_free(struct mlxsw_sp *mlxsw_sp,
			enum mlxsw_sp_kvdl_entry_type type,
			unsigned int entry_count, int entry_index);
int mlxsw_sp_kvdl_alloc_count_query(struct mlxsw_sp *mlxsw_sp,
				    enum mlxsw_sp_kvdl_entry_type type,
				    unsigned int entry_count,
				    unsigned int *p_alloc_count);

/* spectrum1_kvdl.c */
extern const struct mlxsw_sp_kvdl_ops mlxsw_sp1_kvdl_ops;
int mlxsw_sp1_kvdl_resources_register(struct mlxsw_core *mlxsw_core);

/* spectrum2_kvdl.c */
extern const struct mlxsw_sp_kvdl_ops mlxsw_sp2_kvdl_ops;

struct mlxsw_sp_acl_rule_info {
	unsigned int priority;
	struct mlxsw_afk_element_values values;
	struct mlxsw_afa_block *act_block;
	unsigned int counter_index;
};

struct mlxsw_sp_acl_block;
struct mlxsw_sp_acl_ruleset;

/* spectrum_acl.c */
enum mlxsw_sp_acl_profile {
	MLXSW_SP_ACL_PROFILE_FLOWER,
};

struct mlxsw_afk *mlxsw_sp_acl_afk(struct mlxsw_sp_acl *acl);
struct mlxsw_sp_acl_tcam *mlxsw_sp_acl_to_tcam(struct mlxsw_sp_acl *acl);
struct mlxsw_sp *mlxsw_sp_acl_block_mlxsw_sp(struct mlxsw_sp_acl_block *block);
unsigned int mlxsw_sp_acl_block_rule_count(struct mlxsw_sp_acl_block *block);
void mlxsw_sp_acl_block_disable_inc(struct mlxsw_sp_acl_block *block);
void mlxsw_sp_acl_block_disable_dec(struct mlxsw_sp_acl_block *block);
bool mlxsw_sp_acl_block_disabled(struct mlxsw_sp_acl_block *block);
struct mlxsw_sp_acl_block *mlxsw_sp_acl_block_create(struct mlxsw_sp *mlxsw_sp,
						     struct net *net);
void mlxsw_sp_acl_block_destroy(struct mlxsw_sp_acl_block *block);
int mlxsw_sp_acl_block_bind(struct mlxsw_sp *mlxsw_sp,
			    struct mlxsw_sp_acl_block *block,
			    struct mlxsw_sp_port *mlxsw_sp_port,
			    bool ingress);
int mlxsw_sp_acl_block_unbind(struct mlxsw_sp *mlxsw_sp,
			      struct mlxsw_sp_acl_block *block,
			      struct mlxsw_sp_port *mlxsw_sp_port,
			      bool ingress);
bool mlxsw_sp_acl_block_is_egress_bound(struct mlxsw_sp_acl_block *block);
struct mlxsw_sp_acl_ruleset *
mlxsw_sp_acl_ruleset_lookup(struct mlxsw_sp *mlxsw_sp,
			    struct mlxsw_sp_acl_block *block, u32 chain_index,
			    enum mlxsw_sp_acl_profile profile);
struct mlxsw_sp_acl_ruleset *
mlxsw_sp_acl_ruleset_get(struct mlxsw_sp *mlxsw_sp,
			 struct mlxsw_sp_acl_block *block, u32 chain_index,
			 enum mlxsw_sp_acl_profile profile,
			 struct mlxsw_afk_element_usage *tmplt_elusage);
void mlxsw_sp_acl_ruleset_put(struct mlxsw_sp *mlxsw_sp,
			      struct mlxsw_sp_acl_ruleset *ruleset);
u16 mlxsw_sp_acl_ruleset_group_id(struct mlxsw_sp_acl_ruleset *ruleset);

struct mlxsw_sp_acl_rule_info *
mlxsw_sp_acl_rulei_create(struct mlxsw_sp_acl *acl);
void mlxsw_sp_acl_rulei_destroy(struct mlxsw_sp_acl_rule_info *rulei);
int mlxsw_sp_acl_rulei_commit(struct mlxsw_sp_acl_rule_info *rulei);
void mlxsw_sp_acl_rulei_priority(struct mlxsw_sp_acl_rule_info *rulei,
				 unsigned int priority);
void mlxsw_sp_acl_rulei_keymask_u32(struct mlxsw_sp_acl_rule_info *rulei,
				    enum mlxsw_afk_element element,
				    u32 key_value, u32 mask_value);
void mlxsw_sp_acl_rulei_keymask_buf(struct mlxsw_sp_acl_rule_info *rulei,
				    enum mlxsw_afk_element element,
				    const char *key_value,
				    const char *mask_value, unsigned int len);
int mlxsw_sp_acl_rulei_act_continue(struct mlxsw_sp_acl_rule_info *rulei);
int mlxsw_sp_acl_rulei_act_jump(struct mlxsw_sp_acl_rule_info *rulei,
				u16 group_id);
int mlxsw_sp_acl_rulei_act_terminate(struct mlxsw_sp_acl_rule_info *rulei);
int mlxsw_sp_acl_rulei_act_drop(struct mlxsw_sp_acl_rule_info *rulei);
int mlxsw_sp_acl_rulei_act_trap(struct mlxsw_sp_acl_rule_info *rulei);
int mlxsw_sp_acl_rulei_act_mirror(struct mlxsw_sp *mlxsw_sp,
				  struct mlxsw_sp_acl_rule_info *rulei,
				  struct mlxsw_sp_acl_block *block,
				  struct net_device *out_dev,
				  struct netlink_ext_ack *extack);
int mlxsw_sp_acl_rulei_act_fwd(struct mlxsw_sp *mlxsw_sp,
			       struct mlxsw_sp_acl_rule_info *rulei,
			       struct net_device *out_dev,
			       struct netlink_ext_ack *extack);
int mlxsw_sp_acl_rulei_act_vlan(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_acl_rule_info *rulei,
				u32 action, u16 vid, u16 proto, u8 prio,
				struct netlink_ext_ack *extack);
int mlxsw_sp_acl_rulei_act_count(struct mlxsw_sp *mlxsw_sp,
				 struct mlxsw_sp_acl_rule_info *rulei,
				 struct netlink_ext_ack *extack);
int mlxsw_sp_acl_rulei_act_fid_set(struct mlxsw_sp *mlxsw_sp,
				   struct mlxsw_sp_acl_rule_info *rulei,
				   u16 fid, struct netlink_ext_ack *extack);

struct mlxsw_sp_acl_rule;

struct mlxsw_sp_acl_rule *
mlxsw_sp_acl_rule_create(struct mlxsw_sp *mlxsw_sp,
			 struct mlxsw_sp_acl_ruleset *ruleset,
			 unsigned long cookie,
			 struct netlink_ext_ack *extack);
void mlxsw_sp_acl_rule_destroy(struct mlxsw_sp *mlxsw_sp,
			       struct mlxsw_sp_acl_rule *rule);
int mlxsw_sp_acl_rule_add(struct mlxsw_sp *mlxsw_sp,
			  struct mlxsw_sp_acl_rule *rule);
void mlxsw_sp_acl_rule_del(struct mlxsw_sp *mlxsw_sp,
			   struct mlxsw_sp_acl_rule *rule);
struct mlxsw_sp_acl_rule *
mlxsw_sp_acl_rule_lookup(struct mlxsw_sp *mlxsw_sp,
			 struct mlxsw_sp_acl_ruleset *ruleset,
			 unsigned long cookie);
struct mlxsw_sp_acl_rule_info *
mlxsw_sp_acl_rule_rulei(struct mlxsw_sp_acl_rule *rule);
int mlxsw_sp_acl_rule_get_stats(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_acl_rule *rule,
				u64 *packets, u64 *bytes, u64 *last_use);

struct mlxsw_sp_fid *mlxsw_sp_acl_dummy_fid(struct mlxsw_sp *mlxsw_sp);

int mlxsw_sp_acl_init(struct mlxsw_sp *mlxsw_sp);
void mlxsw_sp_acl_fini(struct mlxsw_sp *mlxsw_sp);

/* spectrum_acl_tcam.c */
struct mlxsw_sp_acl_tcam;
struct mlxsw_sp_acl_tcam_region;

struct mlxsw_sp_acl_tcam_ops {
	enum mlxsw_reg_ptar_key_type key_type;
	size_t priv_size;
	int (*init)(struct mlxsw_sp *mlxsw_sp, void *priv,
		    struct mlxsw_sp_acl_tcam *tcam);
	void (*fini)(struct mlxsw_sp *mlxsw_sp, void *priv);
	size_t region_priv_size;
	int (*region_init)(struct mlxsw_sp *mlxsw_sp, void *region_priv,
			   void *tcam_priv,
			   struct mlxsw_sp_acl_tcam_region *region);
	void (*region_fini)(struct mlxsw_sp *mlxsw_sp, void *region_priv);
	int (*region_associate)(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_acl_tcam_region *region);
	size_t chunk_priv_size;
	void (*chunk_init)(void *region_priv, void *chunk_priv,
			   unsigned int priority);
	void (*chunk_fini)(void *chunk_priv);
	size_t entry_priv_size;
	int (*entry_add)(struct mlxsw_sp *mlxsw_sp,
			 void *region_priv, void *chunk_priv,
			 void *entry_priv,
			 struct mlxsw_sp_acl_rule_info *rulei);
	void (*entry_del)(struct mlxsw_sp *mlxsw_sp,
			  void *region_priv, void *chunk_priv,
			  void *entry_priv);
	int (*entry_activity_get)(struct mlxsw_sp *mlxsw_sp,
				  void *region_priv, void *entry_priv,
				  bool *activity);
};

/* spectrum1_acl_tcam.c */
extern const struct mlxsw_sp_acl_tcam_ops mlxsw_sp1_acl_tcam_ops;

/* spectrum2_acl_tcam.c */
extern const struct mlxsw_sp_acl_tcam_ops mlxsw_sp2_acl_tcam_ops;

/* spectrum_acl_flex_actions.c */
extern const struct mlxsw_afa_ops mlxsw_sp1_act_afa_ops;
extern const struct mlxsw_afa_ops mlxsw_sp2_act_afa_ops;

/* spectrum_acl_flex_keys.c */
extern const struct mlxsw_afk_ops mlxsw_sp1_afk_ops;
extern const struct mlxsw_afk_ops mlxsw_sp2_afk_ops;

/* spectrum_flower.c */
int mlxsw_sp_flower_replace(struct mlxsw_sp *mlxsw_sp,
			    struct mlxsw_sp_acl_block *block,
			    struct tc_cls_flower_offload *f);
void mlxsw_sp_flower_destroy(struct mlxsw_sp *mlxsw_sp,
			     struct mlxsw_sp_acl_block *block,
			     struct tc_cls_flower_offload *f);
int mlxsw_sp_flower_stats(struct mlxsw_sp *mlxsw_sp,
			  struct mlxsw_sp_acl_block *block,
			  struct tc_cls_flower_offload *f);
int mlxsw_sp_flower_tmplt_create(struct mlxsw_sp *mlxsw_sp,
				 struct mlxsw_sp_acl_block *block,
				 struct tc_cls_flower_offload *f);
void mlxsw_sp_flower_tmplt_destroy(struct mlxsw_sp *mlxsw_sp,
				   struct mlxsw_sp_acl_block *block,
				   struct tc_cls_flower_offload *f);

/* spectrum_qdisc.c */
int mlxsw_sp_tc_qdisc_init(struct mlxsw_sp_port *mlxsw_sp_port);
void mlxsw_sp_tc_qdisc_fini(struct mlxsw_sp_port *mlxsw_sp_port);
int mlxsw_sp_setup_tc_red(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct tc_red_qopt_offload *p);
int mlxsw_sp_setup_tc_prio(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct tc_prio_qopt_offload *p);

/* spectrum_fid.c */
int mlxsw_sp_fid_flood_set(struct mlxsw_sp_fid *fid,
			   enum mlxsw_sp_flood_type packet_type, u8 local_port,
			   bool member);
int mlxsw_sp_fid_port_vid_map(struct mlxsw_sp_fid *fid,
			      struct mlxsw_sp_port *mlxsw_sp_port, u16 vid);
void mlxsw_sp_fid_port_vid_unmap(struct mlxsw_sp_fid *fid,
				 struct mlxsw_sp_port *mlxsw_sp_port, u16 vid);
enum mlxsw_sp_rif_type mlxsw_sp_fid_rif_type(const struct mlxsw_sp_fid *fid);
u16 mlxsw_sp_fid_index(const struct mlxsw_sp_fid *fid);
enum mlxsw_sp_fid_type mlxsw_sp_fid_type(const struct mlxsw_sp_fid *fid);
void mlxsw_sp_fid_rif_set(struct mlxsw_sp_fid *fid, struct mlxsw_sp_rif *rif);
enum mlxsw_sp_rif_type
mlxsw_sp_fid_type_rif_type(const struct mlxsw_sp *mlxsw_sp,
			   enum mlxsw_sp_fid_type type);
u16 mlxsw_sp_fid_8021q_vid(const struct mlxsw_sp_fid *fid);
struct mlxsw_sp_fid *mlxsw_sp_fid_8021q_get(struct mlxsw_sp *mlxsw_sp, u16 vid);
struct mlxsw_sp_fid *mlxsw_sp_fid_8021d_get(struct mlxsw_sp *mlxsw_sp,
					    int br_ifindex);
struct mlxsw_sp_fid *mlxsw_sp_fid_rfid_get(struct mlxsw_sp *mlxsw_sp,
					   u16 rif_index);
struct mlxsw_sp_fid *mlxsw_sp_fid_dummy_get(struct mlxsw_sp *mlxsw_sp);
void mlxsw_sp_fid_put(struct mlxsw_sp_fid *fid);
int mlxsw_sp_port_fids_init(struct mlxsw_sp_port *mlxsw_sp_port);
void mlxsw_sp_port_fids_fini(struct mlxsw_sp_port *mlxsw_sp_port);
int mlxsw_sp_fids_init(struct mlxsw_sp *mlxsw_sp);
void mlxsw_sp_fids_fini(struct mlxsw_sp *mlxsw_sp);

/* spectrum_mr.c */
enum mlxsw_sp_mr_route_prio {
	MLXSW_SP_MR_ROUTE_PRIO_SG,
	MLXSW_SP_MR_ROUTE_PRIO_STARG,
	MLXSW_SP_MR_ROUTE_PRIO_CATCHALL,
	__MLXSW_SP_MR_ROUTE_PRIO_MAX
};

#define MLXSW_SP_MR_ROUTE_PRIO_MAX (__MLXSW_SP_MR_ROUTE_PRIO_MAX - 1)

struct mlxsw_sp_mr_route_key;

struct mlxsw_sp_mr_tcam_ops {
	size_t priv_size;
	int (*init)(struct mlxsw_sp *mlxsw_sp, void *priv);
	void (*fini)(void *priv);
	size_t route_priv_size;
	int (*route_create)(struct mlxsw_sp *mlxsw_sp, void *priv,
			    void *route_priv,
			    struct mlxsw_sp_mr_route_key *key,
			    struct mlxsw_afa_block *afa_block,
			    enum mlxsw_sp_mr_route_prio prio);
	void (*route_destroy)(struct mlxsw_sp *mlxsw_sp, void *priv,
			      void *route_priv,
			      struct mlxsw_sp_mr_route_key *key);
	int (*route_update)(struct mlxsw_sp *mlxsw_sp, void *route_priv,
			    struct mlxsw_sp_mr_route_key *key,
			    struct mlxsw_afa_block *afa_block);
};

/* spectrum1_mr_tcam.c */
extern const struct mlxsw_sp_mr_tcam_ops mlxsw_sp1_mr_tcam_ops;

/* spectrum2_mr_tcam.c */
extern const struct mlxsw_sp_mr_tcam_ops mlxsw_sp2_mr_tcam_ops;

#endif
