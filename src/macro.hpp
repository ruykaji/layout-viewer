#ifndef __MACRO_H__
#define __MACRO_H__

#define COPY_CONSTRUCTOR_REMOVE(name) name(const name&) = delete
#define ASSIGN_OPERATOR_REMOVE(name) name& operator=(const name&) = delete

#endif
