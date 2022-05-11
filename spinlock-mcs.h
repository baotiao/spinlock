#ifndef _SPINLOCK_MCS
#define _SPINLOCK_MCS

#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))

#define barrier() asm volatile("": : :"memory")
#define cpu_relax() asm volatile("pause\n": : :"memory")

static inline void *xchg_64(void *ptr, void *x)
{
  __asm__ __volatile__("xchgq %0,%1"
      :"=r" ((unsigned long long) x)
      :"m" (*(volatile long long *)ptr), "0" ((unsigned long long) x)
      :"memory");

  return x;
}

typedef struct mcs_lock_t mcs_lock_t;
struct mcs_lock_t
{
  mcs_lock_t *next;
  int spin;
};
typedef struct mcs_lock_t *mcs_lock;

static inline void lock_mcs(mcs_lock *m, mcs_lock_t *me)
{
  mcs_lock_t *tail;

  me->next = NULL;
  me->spin = 0;

  // 将me 添加到mcs_lock list 末尾
  // 注意xchg_64 是把m 和 me 交换, 然后返回的是的m
  tail = xchg_64(m, me);

  /* No one there? */
  if (!tail) return;

  /* Someone there, need to link in */
  // 将之前tail 指针指向me
  tail->next = me;

  /* Make sure we do the above setting of next. */
  barrier();

  /* Spin on my spin variable */
  // 当前thread 的mcs_lock 就一直在这里spin
  // 直到他的前一个节点把这里的spin=true, 那么它就获得了这个lock
  while (!me->spin) {
    cpu_relax();
  }

  return;
}

static inline void unlock_mcs(mcs_lock *m, mcs_lock_t *me)
{
  /* No successor yet? */
  if (me->next == NULL)
  {
    /* Try to atomically unlock */
    if (cmpxchg(m, me, NULL) == me) return;

    /* Wait for successor to appear */
    while (!me->next) cpu_relax();
  }

  // 当有下一个mcs_lock 在等待的时候, mcs 处理逻辑非常简单
  // 直接设置next->spin = 1, 因为next->spin lock 等待的逻辑是
  // while (!me->spin) pause();
  // 那么这样next mcs_lock 就已经获得lock了
  // 当前lock memory 是如何处理这里其实并没有处理, 这里要么复用
  // 要么内存回收, 这就是内存管理的问题了
  /* Unlock next one */
  me->next->spin = 1; 
}

static inline int trylock_mcs(mcs_lock *m, mcs_lock_t *me)
{
  mcs_lock_t *tail;

  me->next = NULL;
  me->spin = 0;

  /* Try to lock */
  tail = cmpxchg(m, NULL, &me);

  /* No one was there - can quickly return */
  if (!tail) return 0;

  return 1; // Busy
}

#endif
