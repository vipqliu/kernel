/*
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/linkage.h>

#include <asm/cache.h>
#include <asm/asm-uaccess.h>

/*
 * Copy to user space from a kernel buffer (alignment handled by the hardware)
 *
 * Parameters:
 *	x0 - to
 *	x1 - from
 *	x2 - n
 * Returns:
 *	x0 - bytes not copied
 */
	.macro ldrb1 ptr, regB, val
	ldrb  \ptr, [\regB], \val
	.endm

	.macro strb1 ptr, regB, val
	uao_user_alternative 9998f, strb, sttrb, \ptr, \regB, \val
	.endm

	.macro ldrh1 ptr, regB, val
	ldrh  \ptr, [\regB], \val
	.endm

	.macro strh1 ptr, regB, val
	uao_user_alternative 9997f, strh, sttrh, \ptr, \regB, \val
	.endm

	.macro ldr1 ptr, regB, val
	ldr \ptr, [\regB], \val
	.endm

	.macro str1 ptr, regB, val
	uao_user_alternative 9997f, str, sttr, \ptr, \regB, \val
	.endm

	.macro ldp1 ptr, regB, regC, val
	ldp \ptr, \regB, [\regC], \val
	.endm

	.macro stp1 ptr, regB, regC, val
	uao_stp 9997f, \ptr, \regB, \regC, \val
	.endm

end	.req	x5
srcin	.req	x15
ENTRY(__arch_copy_to_user)
	uaccess_enable_not_uao x3, x4, x5
	add	end, x0, x2
	mov	srcin, x1
#include "copy_template.S"
	uaccess_disable_not_uao x3, x4
	mov	x0, #0
	ret
ENDPROC(__arch_copy_to_user)

#ifdef CONFIG_UCE_KERNEL_RECOVERY
ENTRY(__arch_copy_to_user_generic_read)
	uaccess_enable_not_uao x3, x4, x5
	add	end, x0, x2
	mov	srcin, x1
#include "copy_template_generic_read.S"
	uaccess_disable_not_uao x3, x4

	mov     x0, #0
	ret

	.global copy_generic_read_sea_fallback
copy_generic_read_sea_fallback:
	uaccess_disable_not_uao x3, x4

	mov	x0, #-1
	ret
ENDPROC(__arch_copy_to_user_generic_read)
#endif

	.section .fixup,"ax"
	.align	2
9997:	cmp	dst, dstin
	b.ne	9998f
	// Before being absolutely sure we couldn't copy anything, try harder
	ldrb	tmp1w, [srcin]
USER(9998f, sttrb tmp1w, [dst])
	add	dst, dst, #1
9998:	sub	x0, end, dst			// bytes not copied
	uaccess_disable_not_uao x3, x4
	ret
	.previous
