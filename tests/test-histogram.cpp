#include <gtest/gtest.h>
#include <printf.h>
#include <cinttypes>

#include "std/histogram.h"

TEST(HistogramTest, SimpleTetst) {
        struct histogram_builder builder;
        struct histogram hist;
        u32 n;
        u32 vbuff;
        float w;

        EXPECT_TRUE(histogram_builder_create(&builder));
        EXPECT_TRUE(histogram_builder_add(&builder, 0));
        EXPECT_TRUE(histogram_builder_add(&builder, 1));
        EXPECT_TRUE(histogram_builder_add(&builder, 1));
        EXPECT_TRUE(histogram_builder_add(&builder, 2));
        EXPECT_TRUE(histogram_builder_add(&builder, 3));
        EXPECT_TRUE(histogram_builder_add(&builder, 4));
        EXPECT_TRUE(histogram_builder_print(stdout, &builder));
        printf("\n");
        EXPECT_TRUE(histogram_builder_build(&hist, &builder, 3));
        EXPECT_TRUE(histogram_print(stdout, &hist));
        printf("\n");

        EXPECT_TRUE(histogram_get_num_buckets(&n, &hist));
        EXPECT_TRUE(histogram_get_bucket_width(&w, &hist));
        for (u32 i = 0; i < n; i++) {
                EXPECT_TRUE(histogram_get_num_bucket_value(&vbuff, i, &hist, 0));
                printf("[%0.f, %0.f): %" PRIu32 "\n", i * w, (i + 1) * w, vbuff);
        }

        EXPECT_TRUE(histogram_drop(&hist));
        EXPECT_TRUE(histogram_builder_drop(&builder));

}

TEST(HistogramTest, EqualDistBucketNumLarger) {
        struct histogram_builder builder;
        struct histogram hist;
        u32 n;
        u32 vbuff;
        float w;

        EXPECT_TRUE(histogram_builder_create(&builder));
        EXPECT_TRUE(histogram_builder_add(&builder, 0));
        EXPECT_TRUE(histogram_builder_add(&builder, 1));
        EXPECT_TRUE(histogram_builder_add(&builder, 2));
        EXPECT_TRUE(histogram_builder_add(&builder, 3));
        EXPECT_TRUE(histogram_builder_add(&builder, 4));
        EXPECT_TRUE(histogram_builder_add(&builder, 5));
        EXPECT_TRUE(histogram_builder_build(&hist, &builder, 10));
        EXPECT_TRUE(histogram_get_num_buckets(&n, &hist));
        EXPECT_TRUE(histogram_get_bucket_width(&w, &hist));
        EXPECT_EQ(n, 6); /* 10 buckets are requested, num elements < buckets, hence num buckets is equal to num values */

        for (u32 i = 0; i < n; i++) {
                EXPECT_TRUE(histogram_get_num_bucket_value(&vbuff, i, &hist, 0));
                EXPECT_EQ(vbuff, 1);
        }

        EXPECT_TRUE(histogram_drop(&hist));
        EXPECT_TRUE(histogram_builder_drop(&builder));
}

TEST(HistogramTest, EqualDistBucketNumEq) {
        struct histogram_builder builder;
        struct histogram hist;
        u32 n;
        u32 vbuff;
        float w;

        EXPECT_TRUE(histogram_builder_create(&builder));
        EXPECT_TRUE(histogram_builder_add(&builder, 0));
        EXPECT_TRUE(histogram_builder_add(&builder, 1));
        EXPECT_TRUE(histogram_builder_add(&builder, 2));
        EXPECT_TRUE(histogram_builder_add(&builder, 3));
        EXPECT_TRUE(histogram_builder_add(&builder, 4));
        EXPECT_TRUE(histogram_builder_add(&builder, 5));
        EXPECT_TRUE(histogram_builder_build(&hist, &builder, 6));
        EXPECT_TRUE(histogram_get_num_buckets(&n, &hist));
        EXPECT_TRUE(histogram_get_bucket_width(&w, &hist));
        EXPECT_EQ(n, 6); /* num buckets equals num values */

        for (u32 i = 0; i < n; i++) {
                EXPECT_TRUE(histogram_get_num_bucket_value(&vbuff, i, &hist, 0));
                EXPECT_EQ(vbuff, 1);
        }

        EXPECT_TRUE(histogram_drop(&hist));
        EXPECT_TRUE(histogram_builder_drop(&builder));
}

TEST(HistogramTest, EqualDistBucketNumSmaller) {
        struct histogram_builder builder;
        struct histogram hist;
        u32 n;
        u32 vbuff;
        float w;

        EXPECT_TRUE(histogram_builder_create(&builder));
        EXPECT_TRUE(histogram_builder_add(&builder, 0));
        EXPECT_TRUE(histogram_builder_add(&builder, 1));
        EXPECT_TRUE(histogram_builder_add(&builder, 2));
        EXPECT_TRUE(histogram_builder_add(&builder, 3));
        EXPECT_TRUE(histogram_builder_add(&builder, 4));
        EXPECT_TRUE(histogram_builder_add(&builder, 5));
        EXPECT_TRUE(histogram_builder_build(&hist, &builder, 3));
        EXPECT_TRUE(histogram_get_num_buckets(&n, &hist));
        EXPECT_TRUE(histogram_get_bucket_width(&w, &hist));
        EXPECT_EQ(n, 3);

        for (u32 i = 0; i < n; i++) {
                EXPECT_TRUE(histogram_get_num_bucket_value(&vbuff, i, &hist, 0));
                EXPECT_EQ(vbuff, 2);
        }

        EXPECT_TRUE(histogram_drop(&hist));
        EXPECT_TRUE(histogram_builder_drop(&builder));
}

TEST(HistogramTest, OneDistBucketNumLarger) {
        struct histogram_builder builder;
        struct histogram hist;
        u32 n;
        u32 vbuff;
        float w;

        EXPECT_TRUE(histogram_builder_create(&builder));
        EXPECT_TRUE(histogram_builder_add(&builder, 0));
        EXPECT_TRUE(histogram_builder_add(&builder, 0));
        EXPECT_TRUE(histogram_builder_add(&builder, 0));
        EXPECT_TRUE(histogram_builder_add(&builder, 0));
        EXPECT_TRUE(histogram_builder_add(&builder, 6));
        EXPECT_TRUE(histogram_builder_add(&builder, 6));
        EXPECT_TRUE(histogram_builder_build(&hist, &builder, 10));
        EXPECT_TRUE(histogram_get_num_buckets(&n, &hist));
        EXPECT_TRUE(histogram_get_bucket_width(&w, &hist));
        EXPECT_EQ(n, 6); /* 10 buckets are requested, num elements < buckets, hence num buckets is equal to num values */

        EXPECT_TRUE(histogram_get_num_bucket_value(&vbuff, 0, &hist, 0));
        EXPECT_EQ(vbuff, 4);
        EXPECT_TRUE(histogram_get_num_bucket_value(&vbuff, 5, &hist, 0));
        EXPECT_EQ(vbuff, 2);

        EXPECT_TRUE(histogram_drop(&hist));
        EXPECT_TRUE(histogram_builder_drop(&builder));
}

TEST(HistogramTest, StressTest) {
        struct histogram_builder builder;
        struct histogram hist;
        u32 n;
        u32 vbuff;
        float w;

        EXPECT_TRUE(histogram_builder_create(&builder));
        for (u32 i = 0; i < 100000; i++) {
                EXPECT_TRUE(histogram_builder_add(&builder, i));
        }
        EXPECT_TRUE(histogram_builder_build(&hist, &builder, 10));
        EXPECT_TRUE(histogram_get_num_buckets(&n, &hist));
        EXPECT_TRUE(histogram_get_bucket_width(&w, &hist));
        EXPECT_EQ(n, 10);

        for (u32 i = 0; i < 10; i++) {
                EXPECT_TRUE(histogram_get_num_bucket_value(&vbuff, i, &hist, 0));
                EXPECT_EQ(vbuff, 10000);
        }

        EXPECT_TRUE(histogram_drop(&hist));
        EXPECT_TRUE(histogram_builder_drop(&builder));
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}