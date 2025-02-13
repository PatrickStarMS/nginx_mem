/*ngx源码移植*/
#include "../include/ngx_mem_pool.h"
#include <stdlib.h>


ngx_mem_pool::ngx_mem_pool(size_t size) {
  if (!ngx_create_pool(size)) {
    printf("create pool failed!\n");
  }
}
ngx_mem_pool::~ngx_mem_pool() {
    ngx_destroy_pool();
    }

bool ngx_mem_pool::ngx_create_pool(size_t size) {
    // ngx_pool_s  *p;


    pool_ = (ngx_pool_s *) malloc(size);
    if (pool_ == nullptr) {
        return false ;
    }

    pool_->d.last = (u_char *) pool_ + sizeof(ngx_pool_s);
    pool_->d.end = (u_char *) pool_ + size;
    pool_->d.next = nullptr;
    pool_->d.failed = 0;

    size = size - sizeof(ngx_pool_s);
    pool_->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    pool_->current = pool_;
    
    pool_->large = nullptr;
    pool_->cleanup = nullptr;
    // pool_ = p;  
    // return p;
    return true;
}

void * ngx_mem_pool::ngx_palloc( size_t size)
{

    if (size <= pool_->max) {
        return ngx_palloc_small(size, 1);
    }

    return ngx_palloc_large(size);
}


void* ngx_mem_pool::ngx_pnalloc(size_t size)
{

    if (size <= pool_->max) {
        return ngx_palloc_small(size, 0);
    }


    return ngx_palloc_large(size);
}

void * ngx_mem_pool::ngx_palloc_small( size_t size, ngx_uint_t align)
{
    u_char      *m;
    ngx_pool_s  *p;

    p = pool_->current;

    do {
        m = p->d.last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return ngx_palloc_block(size);
}


void * ngx_mem_pool::ngx_palloc_block( size_t size)
{
    u_char      *m;
    size_t       psize;
    //newpool是cpp 的关键字，换一个
    ngx_pool_s  *p, *newpool;

    psize = (size_t) (pool_->d.end - (u_char *) pool_);

    m = (u_char *) malloc(psize);
    if (m == nullptr) {
        return nullptr;
    }

    newpool = (ngx_pool_s *) m;

    newpool->d.end = m + psize;
    newpool->d.next = nullptr;
    newpool->d.failed = 0;

    m += sizeof(ngx_pool_data_s);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    newpool->d.last = m + size;

    for (p = pool_->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool_->current = p->d.next;
        }
    }

    p->d.next = newpool;

    return m;
}

void * ngx_mem_pool::ngx_palloc_large( size_t size) 
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_s  *large;
    //ngx_alloc调用的是malloc
    p = (void *) malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    n = 0;

    for (large = pool_->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large =(ngx_pool_large_s *) ngx_palloc_small( sizeof(ngx_pool_large_s), 1);
    if (large == nullptr) {
        free(p);
        return nullptr;
    }

    large->alloc = p;
    large->next = pool_->large;
    pool_->large = large;

    return p;
}
void ngx_mem_pool::ngx_pfree( void *p)
{
    ngx_pool_large_s  *l;

    for (l = pool_->large; l; l = l->next) {
        if (p == l->alloc) {
            
            ngx_free(l->alloc);
            l->alloc = nullptr;

            return ;
        }
    }

    return ;
}
void * ngx_mem_pool::ngx_pcalloc( size_t size)
{
    void *p;

    p = ngx_palloc( size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

void ngx_mem_pool::ngx_destroy_pool()
{
    ngx_pool_s          *p, *n;
    ngx_pool_large_s    *l;
    ngx_pool_cleanup_s  *c;

    for (c = pool_->cleanup; c; c = c->next) {
        if (c->handler) {
            
            c->handler(c->data);
        }
    }


    for (l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    // for (p = pool_, n = pool_->d.next; /* void */; p = n, n = n->d.next) {
    //     ngx_free(p);

    //     if (n == nullptr) {
    //         break;
    //     }
    // }
    //处理第一个内存块
    p = pool_;
    p->d.last = (u_char *) p + sizeof(ngx_pool_s);
    p->d.failed = 0;
    
    //处理第二个内存块
    for(p=pool_->d.next;p!=nullptr;p=p->d.next){
        p->d.last = (u_char *) p + sizeof(ngx_pool_data_s);
        p->d.failed = 0;
    }
}


void ngx_mem_pool::ngx_reset_pool()
{
    ngx_pool_s        *p;
    ngx_pool_large_s  *l;

    for (l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    for (p = pool_; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_s);
        p->d.failed = 0;
    }

    pool_->current = pool_;
  
    pool_->large = nullptr;
}

ngx_pool_cleanup_s * ngx_mem_pool::
ngx_pool_cleanup_add(size_t size)
{
    ngx_pool_cleanup_s  *c;

    c = (ngx_pool_cleanup_s *) ngx_palloc( sizeof(ngx_pool_cleanup_s));
    if (c == nullptr) {
        return nullptr;
    }

    if (size) {
        c->data = ngx_palloc(size);
        if (c->data == nullptr) {
            return nullptr;
        }

    } else {
        c->data = nullptr;
    }

    c->handler = nullptr;
    c->next = pool_->cleanup;

    pool_->cleanup = c;

    return c;
}



typedef struct Data stData;
struct Data
{
    char *ptr;
    FILE *pfile;
};

void func1(char *p1)
{
  char *p = (char *)p1;
  printf("free ptr mem! file:%s line:%d\n",__FILE__,__LINE__) ;
  free(p);
}
void func2(FILE *pf1)
{
  FILE *pf = (FILE *)pf1;
  printf("close file!file:%s line:%d\n",__FILE__,__LINE__);
  fclose(pf);
}
int main() { 
    ngx_mem_pool mempool(512);
    
    // if(!mempool.ngx_create_pool(512)){
    //   printf("create pool failed!\n");
    //   return -1;
    // }//放在构造函数中了

    void *p1 =(stData *) mempool.ngx_palloc(512);
    if(nullptr==p1){
      printf("palloc failed!\n");
      return -1;
    }
    stData *p2 = (stData *)mempool.ngx_palloc(sizeof(stData));
    if(nullptr==p2){     
      printf("palloc failed!\n");     
      return -1;
    }
    p2->ptr = (char *)malloc(12);
    strcpy(p2->ptr, "hello world");
    p2->pfile = fopen("data.txt", "w");
    
    ngx_pool_cleanup_s *c1 =mempool.ngx_pool_cleanup_add( sizeof(char*));
    c1->handler = (ngx_pool_cleanup_pt)func1;
    c1->data = p2->ptr;

    ngx_pool_cleanup_s *c2 = mempool.ngx_pool_cleanup_add( sizeof(FILE*));
    c2->handler =(ngx_pool_cleanup_pt) func2;
    c2->data = p2->pfile;

    // mempool.ngx_destroy_pool();//放在析构函数中了
    return 0;
}