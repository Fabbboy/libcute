#pragma once

#define CU_RC_NAME(NAME) NAME##_Rc
/** Construct an optional helper function name. */
#define CU_RC_FN(NAME, SUFFIX) CU_CONCAT(CU_RC_NAME(NAME), SUFFIX)

#define CU_RC_HEADER(NAME, T)

#define CU_RC_CONTAINER(NAME, T)                                               \
  typedef struct {                                                             \
    T item;                                                                    \
    size_t ref_count;                                                          \
  } CU_RC_FN(NAME, Container);

// ```rs
//  let rc: Rc<i32> = Rc::new(42);
//  let rc_clone = rc.clone();
// ```

#define CU_RC_DECL(NAME, T)                                                    \
  CU_RC_CONTAINER(NAME, T)                                                  \
  typedef struct {                                                             \
    CU_RC_FN(NAME, Container) *inner;
    cu_Allocator alloc;
  } CU_RC_NAME(NAME);                                                        \