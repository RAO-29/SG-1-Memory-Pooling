#include <gtest/gtest.h>
#include <printf.h>
#include <cinttypes>
#include "shared/common.h"
#include "shared/types.h"
#include "core/mem/pool.h"

//void test_mempool_none(struct pool *pool, struct pool_counters *counters)
//{
//        /* test alloc */
//        {
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 0);
//                EXPECT_EQ(counters->num_realloc_calls, 0);
//                EXPECT_EQ(counters->num_free_calls, 0);
//
//                for (u32 j = 1; j < 10000; j++) {
//                        pool_alloc(pool, j);
//                }
//
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 9999);
//
//                pool_free_all(pool);
//
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 9999);
//                EXPECT_EQ(counters->num_free_calls, 9999);
//        }
//
//        /* test free */
//        {
//                pool_reset_counters(pool);
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 0);
//                EXPECT_EQ(counters->num_realloc_calls, 0);
//                EXPECT_EQ(counters->num_free_calls, 0);
//
//                struct vector ofType(void *) vec;
//                vec_create(&vec, NULL, sizeof(void *), 10000);
//
//                for (u32 j = 1; j < 10000; j++) {
//                        void *p = pool_alloc(pool, j);
//                        vec_push(&vec, &p, 1);
//
//                }
//
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 9999);
//
//                for (u32 j = 0; j < vec.num_elems; j++) {
//                        void *p = *vec_get(&vec, j, void *);
//                        pool_free(pool, p);
//                }
//
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 9999);
//                EXPECT_EQ(counters->num_free_calls, 9999);
//
//                vec_drop(&vec);
//        }
//
//        /* test realloc */
//        {
//                pool_reset_counters(pool);
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 0);
//                EXPECT_EQ(counters->num_realloc_calls, 0);
//                EXPECT_EQ(counters->num_free_calls, 0);
//
//                struct vector ofType(void *) vec;
//                vec_create(&vec, NULL, sizeof(void *), 10000);
//
//                for (u32 j = 1; j < 10000; j++) {
//                        void *p = pool_alloc(pool, j);
//                        vec_push(&vec, &p, 1);
//
//                }
//
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 9999);
//
//                for (u32 j = 0; j < vec.num_elems; j++) {
//                        void *p = *vec_get(&vec, j, void *);
//                        void *p2 = pool_realloc(pool, p, (j + 1) * 2);
//                        vec_set(&vec, j, &p2);
//                }
//
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_realloc_calls, 9999);
//                EXPECT_EQ(counters->num_managed_realloc_calls, 0);
//
//                for (u32 j = 0; j < vec.num_elems; j++) {
//                        void *p = *vec_get(&vec, j, void *);
//                        pool_free(pool, p);
//                }
//
//                pool_get_counters(counters, pool);
//                EXPECT_EQ(counters->num_alloc_calls, 9999);
//                EXPECT_EQ(counters->num_free_calls, 9999);
//
//                vec_drop(&vec);
//        }
//}
//
//
//TEST(MemPoolTest, SimpleTetst) {
//
//        struct pool pool;
//        struct pool_strategy strategy;
//        struct pool_counters counters;
//
//        for (u32 i = 0; i < pool_get_num_registered_strategies(); i++) {
//                struct pool_register_entry *entry = pool_register + i;
//                entry->_create(&strategy);
//                const char *name = strategy.impl_name;
//
//                printf("*** Test '%s' ***\n", strategy.impl_name);
//                pool_create_by_name(&pool, name);
//
//                if (strcmp(name, POOL_STRATEGY_NONE_NAME) == 0) {
//                        test_mempool_none(&pool, &counters);
//                } else {
//                        fprintf(stderr, "no test-case implementation found for strategy '%s'\n", name);
//                        EXPECT_TRUE(false);
//                }
//
//                ng5_optional_call(entry, _drop, &strategy);
//
//        }
//}

// TODO: add pool tests again

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}