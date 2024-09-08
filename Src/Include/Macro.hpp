#ifndef __MACRO_HPP__
#define __MACRO_HPP__

/**
 * @brief Removes copy ability from class/structure
 *
 */
#define NON_COPYABLE(class_name)                                                                                                           \
  class_name(class_name&)            = delete;                                                                                             \
  class_name& operator=(class_name&) = delete;

/** Removes move ability from class/structure */
#define NON_MOVABLE(class_name)                                                                                                            \
  class_name(class_name&&)            = delete;                                                                                            \
  class_name& operator=(class_name&&) = delete;

#endif