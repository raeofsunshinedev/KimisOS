#include <stdatomic.h>

typedef struct spinlock{
    atomic_flag lock;
}spinlock_t;

inline void spinlock_acquire(spinlock_t spinlock){
    // atomic_compare_exchange_strong(&spinlock.lock, &spinlock.unlocked, spinlock.locked);
    while(atomic_flag_test_and_set_explicit(&spinlock.lock, memory_order_acquire));
}
inline void spinlock_release(spinlock_t spinlock){
    atomic_flag_clear_explicit(&spinlock.lock, memory_order_release);
}