/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/arch_topology.h - arch specific cpu topology information
 */
#ifndef _LINUX_ARCH_TOPOLOGY_H_
#define _LINUX_ARCH_TOPOLOGY_H_

#include <linux/types.h>
#include <linux/percpu.h>

void topology_normalize_cpu_scale(void);

struct device_node;
bool topology_parse_cpu_capacity(struct device_node *cpu_node, int cpu);

DECLARE_PER_CPU(unsigned long, cpu_scale);

struct sched_domain;
static inline
unsigned long topology_get_cpu_scale(struct sched_domain *sd, int cpu)
{
	return per_cpu(cpu_scale, cpu);
}

void topology_set_cpu_scale(unsigned int cpu, unsigned long capacity);

DECLARE_PER_CPU(unsigned long, freq_scale);

static inline
unsigned long topology_get_freq_scale(int cpu)
{
	return per_cpu(freq_scale, cpu);
}

#ifdef CONFIG_ARCH_GET_PREFERRED_SIBLING_CPUMASK
void arch_get_preferred_sibling_cpumask(unsigned int sibling,
					cpumask_var_t dstp);
#else
static inline void
arch_get_preferred_sibling_cpumask(unsigned int sibling, cpumask_var_t dstp)
{
	if (dstp)
		cpumask_clear(dstp);
}
#endif

#endif /* _LINUX_ARCH_TOPOLOGY_H_ */
