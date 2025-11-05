#pragma once
// Tiny helpers used during Phase-0 experiments (optional)
#define BIT(n)              (1u << (n))
#define REG_SET(reg, mask)  ((reg) |= (mask))
#define REG_CLR(reg, mask)  ((reg) &= ~(mask))
#define REG_BIC(reg, mask)  ((reg) &= ~(mask))
#define REG_BIS(reg, mask)  ((reg) |= (mask))
