#include <gtest/gtest.h>
#include <printf.h>
#include <cinttypes>

#include "core/ptrs/data_ptr.h"

TEST(DataPointerTest, TestPointerEquality) {
        data_ptr_t data_ptr;
        int my_ptr_to_int = 42;
        data_ptr_create(&data_ptr, &my_ptr_to_int);
        int *deref = (int *) data_ptr_get_pointer(data_ptr);
        EXPECT_EQ(&my_ptr_to_int, deref);
        EXPECT_EQ(my_ptr_to_int, *deref);
}

TEST(DataPointerTest, TestPointerUpdate) {
        data_ptr_t data_ptr;
        int my_ptr_to_int = 42;
        int my_ptr_to_int_2 = 43;
        data_ptr_create(&data_ptr, &my_ptr_to_int);
        data_ptr_update(&data_ptr, &my_ptr_to_int_2);
        int *deref = data_ptr_get(int, data_ptr);
        EXPECT_EQ(&my_ptr_to_int_2, deref);
        EXPECT_EQ(my_ptr_to_int_2, *deref);
}

TEST(DataPointerTest, TestAddData) {
        data_ptr_t data_ptr;
        int my_ptr_to_int = 42;
        data_ptr_create(&data_ptr, &my_ptr_to_int);
        data_ptr_set_data(&data_ptr, 23);
        int *deref = (int *) data_ptr_get_pointer(data_ptr);
        EXPECT_EQ(&my_ptr_to_int, deref);
        EXPECT_EQ(my_ptr_to_int, *deref);
}

TEST(DataPointerTest, TestGetData) {
        data_ptr_t data_ptr;
        int my_ptr_to_int = 42;
        u16 stored_data;
        data_ptr_create(&data_ptr, &my_ptr_to_int);
        data_ptr_set_data(&data_ptr, 23);
        int *deref = (int *) data_ptr_get_pointer(data_ptr);
        data_ptr_get_data(&stored_data, data_ptr);
        EXPECT_EQ(&my_ptr_to_int, deref);
        EXPECT_EQ(my_ptr_to_int, *deref);
        EXPECT_EQ(stored_data, 23);
}

TEST(DataPointerTest, TestGetDataAfterPointerUpdate) {
        data_ptr_t data_ptr;
        int my_ptr_to_int = 42;
        int my_ptr_to_int_2 = 43;
        u16 stored_data;
        data_ptr_create(&data_ptr, &my_ptr_to_int);
        data_ptr_set_data(&data_ptr, 23);
        data_ptr_update(&data_ptr, &my_ptr_to_int_2);
        int *deref = (int *) data_ptr_get_pointer(data_ptr);
        data_ptr_get_data(&stored_data, data_ptr);
        EXPECT_EQ(&my_ptr_to_int_2, deref);
        EXPECT_EQ(my_ptr_to_int_2, *deref);
        EXPECT_EQ(stored_data, 23);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}