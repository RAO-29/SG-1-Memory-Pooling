#include <gtest/gtest.h>
#include <printf.h>
#include <cinttypes>

#include "core/ptrs/tagged_ptr.h"

TEST(TaggedPointerTest, TestPointerEquality) {
        tagged_ptr_t tagged_ptr;
        int my_ptr_to_int = 42;
        tagged_ptr_create(&tagged_ptr, &my_ptr_to_int);
        int *deref = (int *) tagged_ptr_get_pointer(tagged_ptr);
        EXPECT_EQ(&my_ptr_to_int, deref);
        EXPECT_EQ(my_ptr_to_int, *deref);
}

TEST(TaggedPointerTest, TestPointerUpdate) {
        tagged_ptr_t tagged_ptr;
        int my_ptr_to_int = 42;
        int my_ptr_to_int_2 = 43;
        tagged_ptr_create(&tagged_ptr, &my_ptr_to_int);
        tagged_ptr_update(&tagged_ptr, &my_ptr_to_int_2);
        int *deref = tagged_ptr_deref(int, tagged_ptr);
        EXPECT_EQ(&my_ptr_to_int_2, deref);
        EXPECT_EQ(my_ptr_to_int_2, *deref);
}

TEST(TaggedPointerTest, TestAddTag) {
        tagged_ptr_t tagged_ptr;
        int my_ptr_to_int = 42;
        u3 tag = 2;
        tagged_ptr_create(&tagged_ptr, &my_ptr_to_int);
        tagged_ptr_set_tag(&tagged_ptr, tag);
        int *deref = (int *) tagged_ptr_get_pointer(tagged_ptr);
        EXPECT_EQ(&my_ptr_to_int, deref);
        EXPECT_EQ(my_ptr_to_int, *deref);
}

TEST(TaggedPointerTest, TestGetTag) {
        tagged_ptr_t tagged_ptr;
        int my_ptr_to_int = 42;
        u3 tag = 2;
        u3 stored_tag;
        tagged_ptr_create(&tagged_ptr, &my_ptr_to_int);
        tagged_ptr_set_tag(&tagged_ptr, tag);
        int *deref = (int *) tagged_ptr_get_pointer(tagged_ptr);
        tagged_ptr_get_tag(&stored_tag, tagged_ptr);
        EXPECT_EQ(&my_ptr_to_int, deref);
        EXPECT_EQ(my_ptr_to_int, *deref);
        EXPECT_EQ(stored_tag, tag);
}

TEST(TaggedPointerTest, TestGetTagAfterPointerUpdate) {
        tagged_ptr_t tagged_ptr;
        int my_ptr_to_int = 42;
        int my_ptr_to_int_2 = 43;
        u3 tag = 2;
        u3 stored_tag;
        tagged_ptr_create(&tagged_ptr, &my_ptr_to_int);
        tagged_ptr_set_tag(&tagged_ptr, tag);
        tagged_ptr_update(&tagged_ptr, &my_ptr_to_int_2);
        int *deref = (int *) tagged_ptr_get_pointer(tagged_ptr);
        tagged_ptr_get_tag(&stored_tag, tagged_ptr);
        EXPECT_EQ(&my_ptr_to_int_2, deref);
        EXPECT_EQ(my_ptr_to_int_2, *deref);
        EXPECT_EQ(stored_tag, tag);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}