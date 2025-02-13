#ifndef A7C86A49_8C17_4EB4_9651_9D0A544F505E
#define A7C86A49_8C17_4EB4_9651_9D0A544F505E
/*
1.移植ngx_mem_pool内存池代码，用oop来实现，ngx源码实现依赖ngx_pool_t,因此要作为成员变量
2.cpp中的结构体在使用的时候，可以不用struct，cpp将其当成一个类
3.移植ngx的头文件，写成cpp的头文件

*/

#include <stdio.h>
#include <cstdlib>
#include <string.h>
//前置声明
struct ngx_pool_s ;//用到了下面定义的ngx_pool_s


// 类型定义
using u_char = unsigned char;//cpp中使用unsigned char
using ngx_uint_t = unsigned int;
using uintptr_t = unsigned long;
// 清理函数的类型（回调函数）
typedef void (*ngx_pool_cleanup_pt)(void *data);
struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;//函数指针，保存清理操作的回调函数
    void                 *data;//传递给回调函数的参数
    ngx_pool_cleanup_s   *next;//所有的hander都会被串在一条链表上
};

//大块内存的头部信息
struct ngx_pool_large_s {
    ngx_pool_large_s     *next;
    void                 *alloc;//通用指针
};

//小块内存的头部信息
struct ngx_pool_data_s {
  u_char* last;
  u_char* end;
  ngx_pool_s* next;
  ngx_uint_t failed;
};
// ngx内存的头部信息和管理成员信息
struct ngx_pool_s {
  ngx_pool_data_s d;
  size_t max;
  ngx_pool_s* current;
//   ngx_chain_t* chain;
  ngx_pool_large_s* large;
  ngx_pool_cleanup_s* cleanup;
//   ngx_log_t* log;
};
//辅助函数，宏定义
#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
//把指针p对齐到a的临近倍数
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))
#define ngx_free free;
//清零
#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
// 小块内存对齐最小单位
const int NGX_ALIGNMENT = sizeof(unsigned long);
// 默认物理一个页面大小4k
const int ngx_pagesize = 4096;
//ngx小块内存池可分配最大空间
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1;
//默认的ngx内存池大小
const int NGX_DEFAULT_POOL_SIZE = 16*1024;//16k
//内存池大小按照16字节对齐
const int NGX_POOL_ALIGNMENT = 16;
//ngx小内存池的最小四则调整成NGX_MIN_POOL_ALIGNMENT的邻近的倍数 
const int NGX_MIN_POOL_SIZE = ngx_align((sizeof(ngx_pool_s)+2*sizeof(ngx_pool_large_s) ), NGX_POOL_ALIGNMENT);
//将内存池封装成对象，函数变成成员方法，结构体变成成员变量

//可用大小size-sizeof(ngx_pool_s)

class ngx_mem_pool {
 public:
  ngx_mem_pool(size_t size);  //构造函数
  ~ngx_mem_pool();  
 //指定size大小的内存池，但是小块大小的内存池不超过一个页面的大小
  bool ngx_create_pool(size_t size);
  //考虑内存对齐，分配size大小的内存
  void *ngx_palloc(size_t size);
  //不考虑内存对齐，分配size大小的内存
  void *ngx_pnalloc(size_t size);
  //调用的是ngx_palloc,但是会初始化为0
  void *ngx_pcalloc(size_t size);
  //释放大内存块
  void ngx_pfree(void *p);
  //内存池重置
  void ngx_reset_pool();
  //内存池销毁
  void ngx_destroy_pool();
  //添加回调清理回调函数
  ngx_pool_cleanup_s* ngx_pool_cleanup_add(size_t size);      
 private:
  ngx_pool_s* pool_;  // 指向内存池的入口指针
  //小块内存分配
  void *ngx_palloc_small(size_t size, ngx_uint_t align);
  //大块内存分配  
  void *ngx_palloc_large(size_t size);
  //分配新的小块内存池
  void *ngx_palloc_block(size_t size);
};





#endif /* A7C86A49_8C17_4EB4_9651_9D0A544F505E */
