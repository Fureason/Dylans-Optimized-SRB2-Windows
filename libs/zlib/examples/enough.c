/* enough.c -- determine the maximum size of inflate's Huffman code tables over
 * all possible valid and complete prefix codes, subject to a length limit.
 * Copyright (C) 2007, 2008, 2012, 2018 Mark Adler
 * Version 1.5  5 August 2018  Mark Adler
 */

                   
                                                                         
                                                                   
                                                                        
                                      
                                                                   
                                                                   
                                                             
                                                                   
                                                             
                                                                               
                                                            
                                              
                                                                  
                                                                             
   

  
                                                                        
                                                                             
                                                   

                                                                              
                                                                             
                                                                              
                                                                       

                                                                               
                                                                             
                                                                             
                                                                               
                                                                         
                                                                        
                                                                           
                                                                               
                                                                              
          

                                                                          
                                                                             
                                                                           
                                                                              
                                                                            
                                                                        
                                                                               
                                                                              
                                                                              
                                                                            
                                                                       
                                                                             
                                                                               
                                                                              
                                                                             
                                         

                                                                            
                                                                            
                                                                           
                                                                              
                                                           

                                                                             
                                                                            
                                                                        
                                                                               
                                                                             
                        

                                                                     
                                                                              
                                                                              
                                                                           
                                                                            
                                                                           
                                                                              
                                                                           
                                                                              
                                                                           
                                                                         
                                                                       
                                                                           
                                                                            
                                                                    

                                                                         
                                                                              
                                                                               
                                                                             
                                                                         
         

                                                                               
                                                                              
                                                                             
                                                                               
                        
   

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>

#define local static


typedef uintmax_t big_t;     
#define PRIbig "ju"          
typedef uintmax_t code_t;    
struct tab {                 
    size_t len;              
    char *vec;               
};

                                                                     

                                               
                                                          
                                                               

                                                            

                                                       
                                                                     
                                                          

                                                                             
                                                                            
                                                                              
                                                                          
                                                                             
                                                                              

                                                                               
                                                                               
                                                                            
                                                                         
                                                                             
                                                                          

                                                                            
                                                                             
                                                                       
                                                                         
   

                                                                               
                                                                           
                                                                          
                                                                             
                                                                          
                                                                             
                                                                         
                                                                               
                                                                           
                                                                            
                                                                            
                                                                         
                                                                  

                                                                          
                                                                             
   


typedef struct {
    char *str;           
    size_t size;         
    size_t len;          
} string_t;


local void string_clear(string_t *s) {
    s->str[0] = 0;
    s->len = 0;
}


local void string_init(string_t *s) {
    s->size = 16;
    s->str = malloc(s->size);
    assert(s->str != NULL && "out of memory");
    string_clear(s);
}


local void string_free(string_t *s) {
    free(s->str);
    s->str = NULL;
    s->size = 0;
    s->len = 0;
}



local void string_printf(string_t *s, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    size_t len = s->len;
    int ret = vsnprintf(s->str + len, s->size - len, fmt, ap);
    assert(ret >= 0 && "out of memory");
    s->len += ret;
    if (s->size < s->len + 1) {
        do {
            s->size <<= 1;
            assert(s->size != 0 && "overflow");
        } while (s->size < s->len + 1);
        s->str = realloc(s->str, s->size);
        assert(s->str != NULL && "out of memory");
        vsnprintf(s->str + len, s->size - len, fmt, ap);
    }
    va_end(ap);
}


struct {
    int max;             
    int root;            
    int large;           
    size_t size;         
    big_t tot;           
    string_t out;        
    int *code;           
    big_t *num;          
    struct tab *done;    
} g;


local inline size_t map(int syms, int left, int len) {
    return ((size_t)((syms - 1) >> 1) * ((syms - 2) >> 1) +
            (left >> 1) - 1) * (g.max - 1) +
           len - 1;
}


local void cleanup(void) {
    if (g.done != NULL) {
        for (size_t n = 0; n < g.size; n++)
            if (g.done[n].len)
                free(g.done[n].vec);
        g.size = 0;
        free(g.done);   g.done = NULL;
    }
    free(g.num);    g.num = NULL;
    free(g.code);   g.code = NULL;
    string_free(&g.out);
}





local big_t count(int syms, int left, int len) {

    if (syms == left)
        return 1;


    assert(syms > left && left > 0 && len < g.max);


    size_t index = map(syms, left, len);
    big_t got = g.num[index];
    if (got)
        return got;          



    int least = (left << 1) - syms;
    if (least < 0)
        least = 0;




    int most = (((code_t)left << (g.max - len)) - syms) /
               (((code_t)1 << (g.max - len)) - 1);


    big_t sum = 0;
    for (int use = least; use <= most; use++) {
        got = count(syms - use, (left - use) << 1, len + 1);
        sum += got;
        if (got == (big_t)-1 || sum < got)       
            return (big_t)-1;
    }


    assert(sum != 0);


    g.num[index] = sum;
    return sum;
}





local int been_here(int syms, int left, int len, int mem, int rem) {

    size_t index = map(syms, left, len);
    mem -= 1 << g.root;              
    mem >>= 1;                       
    rem >>= 1;
    size_t offset = (mem >> 3) + rem;
    offset = ((offset * (offset + 1)) >> 1) + rem;
    int bit = 1 << (mem & 7);


    size_t length = g.done[index].len;
    if (offset < length && (g.done[index].vec[offset] & bit) != 0)
        return 1;        




    if (length <= offset) {

        char *vector;
        if (length) {
            do {
                length <<= 1;
            } while (length <= offset);
            vector = realloc(g.done[index].vec, length);
            assert(vector != NULL && "out of memory");
            memset(vector + g.done[index].len, 0, length - g.done[index].len);
        }


        else {
            length = 16;
            while (length <= offset)
                length <<= 1;
            vector = calloc(length, 1);
            assert(vector != NULL && "out of memory");
        }


        g.done[index].len = length;
        g.done[index].vec = vector;
    }


    g.done[index].vec[offset] |= bit;
    return 0;
}





local void examine(int syms, int left, int len, int mem, int rem) {

    if (syms == left) {

        g.code[len] = left;


        while (rem < left) {
            left -= rem;
            rem = 1 << (len - g.root);
            mem += rem;
        }
        assert(rem == left);


        if (mem >= g.large) {


            if (mem > g.large) {
                g.large = mem;
                string_clear(&g.out);
            }


            syms = 0;
            left = 1 << g.max;
            for (int bits = g.max; bits > g.root; bits--) {
                syms += g.code[bits];
                left -= g.code[bits];
                assert((left & 1) == 0);
                left >>= 1;
            }


            string_printf(&g.out, "<%u, %u, %u>:",
                          syms, g.root + 1, ((1 << g.root) - left) << 1);
            for (int bits = g.root + 1; bits <= g.max; bits++)
                if (g.code[bits])
                    string_printf(&g.out, " %d[%d]", g.code[bits], bits);
            string_printf(&g.out, "\n");
        }


        g.code[len] = 0;
        return;
    }


    if (been_here(syms, left, len, mem, rem))
        return;



    int least = (left << 1) - syms;
    if (least < 0)
        least = 0;




    int most = (((code_t)left << (g.max - len)) - syms) /
               (((code_t)1 << (g.max - len)) - 1);


    int use = least;
    while (rem < use) {
        use -= rem;
        rem = 1 << (len - g.root);
        mem += rem;
    }
    rem -= use;


    for (use = least; use <= most; use++) {
        g.code[len] = use;
        examine(syms - use, (left - use) << 1, len + 1,
                mem + (rem ? 1 << (len - g.root) : 0), rem << 1);
        if (rem == 0) {
            rem = 1 << (len - g.root);
            mem += rem;
        }
        rem--;
    }


    g.code[len] = 0;
}






local void enough(int syms) {

    for (int n = 0; n <= g.max; n++)
        g.code[n] = 0;


    string_clear(&g.out);            
    g.large = 1 << g.root;           
    if (g.root < g.max)              
        for (int n = 3; n <= syms; n++)
            for (int left = 2; left < n; left += 2) {


                size_t index = map(n, left, g.root + 1);
                if (g.root + 1 < g.max && g.num[index])  
                    examine(n, left, g.root + 1, 1 << g.root, 0);



                if (g.num[index - 1] && n <= left << 1)
                    examine((n - left) << 1, (n - left) << 1, g.root + 1,
                            1 << g.root, 0);
            }


    printf("maximum of %d table entries for root = %d\n", g.large, g.root);
    fputs(g.out.str, stdout);
}
















int main(int argc, char **argv) {

    g.code = NULL;
    g.num = NULL;
    g.done = NULL;
    string_init(&g.out);


    int syms = 286;
    g.root = 9;
    g.max = 15;
    if (argc > 1) {
        syms = atoi(argv[1]);
        if (argc > 2) {
            g.root = atoi(argv[2]);
            if (argc > 3)
                g.max = atoi(argv[3]);
        }
    }
    if (argc > 4 || syms < 2 || g.root < 1 || g.max < 1) {
        fputs("invalid arguments, need: [sym >= 2 [root >= 1 [max >= 1]]]\n",
              stderr);
        return 1;
    }


    if (g.max > syms - 1)
        g.max = syms - 1;


    int bits = 0;
    for (code_t word = 1; word; word <<= 1)
        bits++;


    if (g.max > bits || (code_t)(syms - 2) >= ((code_t)-1 >> (g.max - 1))) {
        fputs("abort: code length too long for internal types\n", stderr);
        return 1;
    }


    if ((code_t)(syms - 1) > ((code_t)1 << g.max) - 1) {
        fprintf(stderr, "%d symbols cannot be coded in %d bits\n",
                syms, g.max);
        return 1;
    }


    g.code = calloc(g.max + 1, sizeof(int));
    assert(g.code != NULL && "out of memory");



    if (syms == 2)               
        g.num = NULL;            
    else {
        g.size = syms >> 1;
        int n = (syms - 1) >> 1;
        assert(g.size <= (size_t)-1 / n && "overflow");
        g.size *= n;
        n = g.max - 1;
        assert(g.size <= (size_t)-1 / n && "overflow");
        g.size *= n;
        g.num = calloc(g.size, sizeof(big_t));
        assert(g.num != NULL && "out of memory");
    }


    big_t sum = 0;
    for (int n = 2; n <= syms; n++) {
        big_t got = count(n, 2, 1);
        sum += got;
        assert(got != (big_t)-1 && sum >= got && "overflow");
    }
    printf("%"PRIbig" total codes for 2 to %d symbols", sum, syms);
    if (g.max < syms - 1)
        printf(" (%d-bit length limit)\n", g.max);
    else
        puts(" (no length limit)");


    if (syms == 2)
        g.done = NULL;
    else {
        g.done = calloc(g.size, sizeof(struct tab));
        assert(g.done != NULL && "out of memory");
    }


    if (g.root > g.max)              
        g.root = g.max;
    if ((code_t)syms < ((code_t)1 << (g.root + 1)))
        enough(syms);
    else
        fputs("cannot handle minimum code lengths > root", stderr);


    cleanup();
    return 0;
}
