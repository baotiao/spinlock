#ifndef _SPINLOCK_TICKET_H
#define _SPINLOCK_H

/* Code copied from http://locklessinc.com/articles/locks/ */

#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))

#define barrier() asm volatile("": : :"memory")
#define cpu_relax() asm volatile("pause\n": : :"memory")

#define spin_lock ticket_lock
#define spin_unlock ticket_unlock
#define spinlock ticketlock

#define SPINLOCK_INITIALIZER { 0, 0 };

typedef union ticketlock ticketlock;

union ticketlock
{
  unsigned u;
  struct
  {
    unsigned short ticket;
    unsigned short users;
  } s;
};

// ticket_lock 和 MCS Lock 原理一样, 只不过MCS 是通过指针指向next
// 而ticket_lock 则是通过ticket 来判断自己是不是next
// 理论上 ticket_lock 性能应该不如MCS 吧
static inline void ticket_lock(ticketlock *t)
{
  // 初始的时候 ticket = 0, users = 0
  // 如果没有人持有lock, 那么此时
  // t->s.users = 1. me = 0 
  unsigned short me = atomic_xadd(&t->s.users, 1);

  // 那么这里判断 t->s.ticket = 0, me = 0
  // 那么就可以获得这个lock
  // 此时如果另外一个thread 也过来获得这个ticket
  // 那么上面 t->s.users = 2, me = 1
  // 而此时因为前一个lock 还没有unlock, 所以ticket 依然等于0
  // 这里t->s.tiket = 0 而 me = 1 那么就无法获得lock
  int wait = 1;
  while (t->s.ticket != me) {
    // wait here is important to performance.
    for (int i = 0; i < wait; i++) {
      cpu_relax();
    }
    wait *= 2;
    if (wait > 8) {
      wait = 8;
    }
      // cpu_relax();
      // cpu_relax();
      // cpu_relax();
      // cpu_relax();
      // cpu_relax();
      // cpu_relax();
      // cpu_relax();
      // cpu_relax();
  }
}

static inline void ticket_unlock(ticketlock *t)
{
  barrier();
  // lock release 的时候ticket++, 那么下一个lock wait 的人就可以获得Lock 了
  t->s.ticket++;
}

static inline int ticket_trylock(ticketlock *t)
{
  unsigned short me = t->s.users;
  unsigned short menew = me + 1;
  unsigned cmp = ((unsigned) me << 16) + me;
  unsigned cmpnew = ((unsigned) menew << 16) + me;

  if (cmpxchg(&t->u, cmp, cmpnew) == cmp) return 0;

  return 1; // Busy
}

static inline int ticket_lockable(ticketlock *t)
{
  ticketlock u = *t;
  barrier();
  return (u.s.ticket == u.s.users);
}

#endif
