#ifndef STARS_ENUMCONSTANTS_H
#define STARS_ENUMCONSTANTS_H

// C has separate tag and typedef namespaces. C++ constants must leave the
// public fixed-width aliases available under their original names.
#ifdef __cplusplus
#define STARS_CONSTANTS(name) enum
#else
#define STARS_CONSTANTS(name) enum name
#endif

#endif
