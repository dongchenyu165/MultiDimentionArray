# TArrayMultiDim

# Intro

`TArrayMultiDim` 是一个用于Unreal Engine处理多维数组的模板类，旨在提供灵活且高效的多维数组操作。它支持动态调整数组维度大小、切片操作以及自定义存储顺序等功能。该类的设计初衷是为了在 Unreal Engine 项目中提供一种比【TArray嵌套】更加便捷的多维数组处理方式。

`TArrayMultiDim` is a template class for handling multi-dimensional arrays in Unreal Engine, designed to provide a flexible and efficient multi-dimensional array operations. It supports dynamic resizing of array dimensions, slicing operations, and custom storage orders. The class is intended to offer a more convenient way to handle multi-dimensional arrays in Unreal Engine projects compared to nested `TArray`.

# Features
- **多维数组支持**：支持任意维度的数组，维度大小可以在编译时或运行时动态指定。
- **自定义存储顺序**：允许用户指定数组的存储顺序，以优化内存访问性能。
- **切片操作**：支持类似于 NumPy 的切片操作，便于获取数组的子集。
- **数据初始化和重置**：提供多种方式初始化和重置数据，包括从嵌套列表初始化和使用自定义函数。
- **通过Mask进行数据的选择**：支持通过掩码（Mask）选择数据，便于进行条件操作。

---

- **Multi-dimensional array support**: Supports arrays of arbitrary dimensions, with dimension sizes that can be dynamically specified at compile-time or runtime.
- **Custom storage order**: Allows users to specify the storage order of the array to optimize memory access performance.
- **Slicing operations**: Supports slicing operations similar to NumPy, making it easy to obtain subsets of the array.
- **Data initialization and reset**: Provides various ways to initialize and reset data, including initialization from nested lists and using custom functions.
- **Data selection via Mask**: Supports data selection via a mask, making it convenient for conditional operations.

# Some definitions
- 坐标：一组整数，用于定位数组中元素的位置。  Coordinate: A sort of int numbers, used to locate the position of an element in the array.
- 线性索引：单个整数，表示后端存储中的数据索引（`TArrayMultiDim`类使用 TArray 作为后端存储）。  Linear index: Single int number, the index of data in the backend storage (`TArrayMultiDim` class use `TArray` for backend storage).
- 存储顺序：参考 [Eigen 库的页面](https://eigen.tuxfamily.org/dox/group__TopicStorageOrders.html)。每个维度的存储优先级。  Storage order: Reference the [Eigen library's page](https://eigen.tuxfamily.org/dox/group__TopicStorageOrders.html). The storage priority for each dimension.


## Storage order
存储顺序决定了元素在底层线性存储中的存储顺序。  
The storage order determines the order in which elements are stored in the underlying linear storage.

For a 3x3x3 array:
```
{
	{
		{1, 2, 3}, 
		{4, 5, 6}, 
		{7, 8, 9}
	},
	{
		{10, 11, 12}, 
		{13, 14, 15}, 
		{16, 17, 18}
	},
	{
		{19, 20, 21}, 
		{22, 23, 24}, 
		{25, 26, 27}
	}
}
```
使用存储顺序 [2, 1, 0] 表示：最内层（第3维，{1, 2, 3}）维度优先级最高，其次是中间维度（第2维，{1, 4, 7}），最外层维度（第1维，{1, 10, 19}）优先级最低。
因此，最终存储顺序如下：  
Use order [2, 1, 0] means the innermost (3rd, {1, 2, 3}) dimension is the highest, followed by the middle dimension (2nd, {1, 4, 7}) and the outer dimension (1st, {1, 10, 19}) is the lowest.
So, the final storage will be like:
```
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 
	10, 11, 12, 13, 14, 15, 16, 17, 18, 
	19, 20, 21, 22, 23, 24, 25, 26, 27
}
```

使用存储顺序 [1, 0, 2] 表示：中间维度（第2维，{1, 4, 7}）优先级最高，其次是最外层维度（第1维，{1, 10, 19}），最内层维度（第3维，{1, 2, 3}）优先级最低。
因此，最终存储顺序如下：
Use the order [1, 0, 2] means the middle dimension (2nd, {1, 4, 7}) is the highest, followed by the outer dimension (1st, {1, 10, 19}) and the innermost dimension (3rd, {1, 2, 3}) is the lowest.
So, the final storage will be like:

```
{
	1, 4, 7, 10, 13, 16, 19, 22, 25, 
	2, 5, 8, 11, 14, 17, 20, 23, 26, 
	3, 6, 9, 12, 15, 18, 21, 24, 27
}
```

但是，storage order只影响在内存中的顺序（即只影响缓存命中），不会影响通过坐标访问的结果。
即：坐标(0, 0, 0) 的值始终是 1，坐标 (1, 1, 1) 的值始终是 14，无论 storage order 如何设置。  
However, the storage order only affects the order in memory (cache hits) and does not affect the result of accessing elements by coordinates.
For example, the value at coordinate (0, 0, 0) is always 1, and the value at coordinate (1, 1, 1) is always 14, regardless of the storage order.

storage order应当根据访问模式来设置，以优化缓存命中率。

# Usages & Examples

## C++

### Construction
Declaration:
```cpp
// Fixed size array, can use std::initializer_list to init the data
ArrayMultiDim::TArrayMultiDim<int, 3, 3> MultiDimArray;  // 2-dimensional array with fixed size
ArrayMultiDim::TArrayMultiDim<int, 3, 3, 3> MultiDimArray;  // 3-dimensional array with fixed size
ArrayMultiDim::TArrayMultiDim<int, 3, 3, 3, 7> MultiDimArray;  // 4-dimensional array with fixed size

// Dynamic size array, can not init the data directly
// 		should use SetDimSize() member function to set the size, before init the data.
ArrayMultiDim::TArrayMultiDim<int, -1, -1> MultiDimArray;  // 2-dimensional array with dynamic size
```

Create a 3-dimensional array from a nested std::initializer_list:
```cpp
#include "ArrayMultiDim.h"

// Define a nested list type with 3 dimensions
using NestedListType = ArrayMultiDim::NestList<3, int>;
NestedListType TestDataList = {
	{
		{1, 2, 3}, 
		{4, 5, 6}, 
		{7, 8, 9}
	},
	{
		{10, 11, 12}, 
		{13, 14, 15}, 
		{16, 17, 18}
	},
	{
		{19, 20, 21}, 
		{22, 23, 24}, 
		{25, 26, 27}
	}
};
{ 
	{
		{1, 2, 3}, 
		{4, 5, 6}, 
		{7, 8, 9}
	},
	{
		{10, 11, 12}, 
		{13, 14, 15}, 
		{16, 17, 18}
	},
	{
		{19, 20, 21}, 
		{22, 23, 24}, 
		{25, 26, 27}
	}
};

// Use defined list to create a 3-dimensional array with the default storage order.
// The default storage order is [ArrayMultiDim::Odr<2, 1, 0>()], means the innermost dimension is the highest, followed by the middle dimension, and the outer dimension is the lowest.
// The data storage with the sequence: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27]
ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimArray_210 {TestDataList};

// Use defined list to create a 3-dimensional array with the SPECIFIED storage order.
// The order [ArrayMultiDim::Odr<1, 0, 2>()] means the priority of the middle dimension is the highest, followed by the outer dimension, and the innermost dimension is the lowest.
// The data storage with the sequence: [1, 4, 7, 10, 13, 16, 19, 22, 25, 2, 5, 8, 11, 14, 17, 20, 23, 26, 3, 6, 9, 12, 15, 18, 21, 24, 27]
ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimArray_102 {TestDataList, ArrayMultiDim::Odr<1, 0, 2>()};

// Of curse, you can init data as follows:
// Default order
ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimArray3_210 {
	{
		{1, 2, 3}, 
		{4, 5, 6}, 
		{7, 8, 9}
	},
	{
		{10, 11, 12}, 
		{13, 14, 15}, 
		{16, 17, 18}
	},
	{
		{19, 20, 21}, 
		{22, 23, 24}, 
		{25, 26, 27}
	}
};
// Specified order. Note that the nesting level of curly braces is one more than above to accommodate the storage order parameter.
ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimArray2_210 {
	{
		{
			{1, 2, 3}, 
			{4, 5, 6}, 
			{7, 8, 9}
		},
		{
			{10, 11, 12}, 
			{13, 14, 15}, 
			{16, 17, 18}
		},
		{
			{19, 20, 21}, 
			{22, 23, 24}, 
			{25, 26, 27}
		}
	},
	ArrayMultiDim::Odr<1, 0, 2>()
};

```


Create a dynamic size array and set its size, also init the data:
The usage procedure:
1. Declare the array with dynamic size.
2. SetDimSize to set the size of the array.
3. SetData to set the data.
```cpp
using DynamicType = ArrayMultiDim::TArrayMultiDim<int, -1, -1, -1>;  // The -1 means the size of the dimension is dynamic.
DynamicType DynamicArray;
DynamicArray.SetDimSize({3, 3, 3});  // Set the size of the array to 3x3x3
DynamicArray.SetDimSize({3, 3, 3}, {1, 0, 2});  // Set the size of the array to 3x3x3, with the specified storage order

// Use callback function to set the data.
DynamicArray.SetData(
	[](const DynamicType::CoordinateType& InCoord, int InLinearIdx, int& InOldData) -> int 
	{
		// It provides the coordinate and linear index of the element, and the old data of the element.
		return InLinearIdx;
	}
);
```

### Resizing
Same as above example, the resizing operation is use the [SetDimSize] member function:


```cpp
// Same as above example
MultiDimArray.SetDimSize({5, 5, 5});
MultiDimArray.SetDimSize({5, 5, 5}, {1, 0, 2});

// Also you can specify the copy policy when resizing:

// The policy [CoordinationCopy] means the data will be copied according to the coordinate.
// 		Like use numpy [:5, :5, :5] to slice the array.
MultiDimArray.SetDimSize({5, 5, 5}, {1, 0, 2}, ArrayMultiDim::EResizeDataCopyPolicy::CoordinationCopy);

// The policy [PreserveOldData] means no data copying, just use [TArray::SetNum] to preserve the array size.
MultiDimArray.SetDimSize({5, 5, 5}, {1, 0, 2}, ArrayMultiDim::EResizeDataCopyPolicy::PreserveOldData);

// The policy [SetToUninitializedValue] means the data will be set to the uninitialized value, by calling TArray's [SetNumUninitialized] function.
MultiDimArray.SetDimSize({5, 5, 5}, {1, 0, 2}, ArrayMultiDim::EResizeDataCopyPolicy::SetToUninitializedValue);

// The policy [SetToInitialValue] means the data will be set to the initial value, by calling TArray's [SetNumZeroed] function.
MultiDimArray.SetDimSize({5, 5, 5}, {1, 0, 2}, ArrayMultiDim::EResizeDataCopyPolicy::SetToInitialValue);
```


### Data accessing
The slicing operation is similar to NumPy, and the slicing result is a dynamic size array:
```cpp
// Create a 2-dim array with the default storage order and init data.
using SliceTestType = ArrayMultiDim::TArrayMultiDim<int, 10, 10>;
SliceTestType MultiDimArray_Slicing {};
auto ResetDataFunc = [](const SliceTestType::CoordinateType& InCoord, int InLinearIdx, int& InOldData) -> int {
	return InLinearIdx;
};
MultiDimArray_Slicing.SetData(ResetDataFunc);
// The data'll be init as follows:
// 00 01 02 03 ... ... 07 08 09
// 10 11 12 13 ... ... 17 18 19
// 20 21 22 23 ... ... 27 28 29
// ... ... ... ... ... ... ... 
// 70 71 72 73 ... ... 77 78 79
// 80 81 82 83 ... ... 87 88 89
// 90 91 92 93 ... ... 97 98 99

// Slice the array with the specified range.
// Equal to Numpy: [:, 2:4]
// Note: [SelfDynamicSizeType] is the dynamic size version of the [SliceTestType] type. If you define a dynamic size array TArrayMultiDim<int, 4, 8, 2>, it's SelfDynamicSizeType will be TArrayMultiDim<int, -1, -1, -1>.
SliceTestType::SelfDynamicSizeType SliceRes = MultiDimArray_Slicing.Slice({{}, {2, 4}});
// The data'll be got as follows:
// 02 03
// 12 13
// 22 23
// 32 33
// 42 43
// 52 53
// 62 63
// 72 73
// 82 83
// 92 93
```

Access single data.
```cpp
// 通过线性索引访问数据
// Loop by the linear storage index.
int value = MultiDimArray[5];

// 通过多维坐标访问数据
// Loop by the multi-dimensional coordinate.
int value = MultiDimArray(1, 2, 3);
```

Loop the data by iterators.
```cpp
// Normal iterator
for (auto It = MultiDimArray.CreateIterator(); It; ++It) 
{
	auto& data = *It;
}
// Const Iterator
for (auto It = MultiDimArray.CreateConstIterator(); It; ++It) 
{
	auto& data = *It;
}

// Use member function.

```

Use member function and callback to iterating data.
```cpp
// Loop by the linear storage index.
auto LoopFunc = [](const SliceTestType::CoordinateType& InCoordinate,
			SliceTestType::IndexType InLinearIdx,
			SliceTestType::IndexType InLoopCount,
			SliceTestType::StorageDataType& InData)
		{
			// Use [InData] to access the data.

			// [InCoordinate] is the coordinate (like [0, 2, 1]) of the data.
			// [InLinearIdx] is the linear storage index of the data.
			// [InLoopCount] is the loop count (may not equal to [InLinearIdx] depending on the storage order).
		};
SliceRes.LoopByIndex(LoopFunc);
// You can also specify the second bool-typed parameter (default is [false]) to decided whether calculating the coordinate of data for handling the coordinate of data.
SliceRes.LoopByIndex(LoopFunc, true);

// Loop by the coordinate with the inner-first order.
// Means this function will iterate the data in the following order (3x3x3 array):
// 		[0, 0, 0]
// 		[0, 0, 1]
// 		[0, 0, 2]
// 		[0, 1, 0]
// 		[0, 1, 1]
// 		[0, 1, 2]
// 		[0, 2, 0]
// 		[0, 2, 1]
// 		[0, 2, 2]
// 		[1, 0, 0]
// 		[1, 0, 1]
// 		[1, 0, 2]
// 		[1, 1, 0]
// 		[1, 1, 1]
// 		[1, 1, 2]
// 		[1, 2, 0]
// 		[1, 2, 1]
// 		[1, 2, 2]
// 		[2, 0, 0]
// 		[2, 0, 1]
// 		[2, 0, 2]
// 		[2, 1, 0]
// 		[2, 1, 1]
// 		[2, 1, 2]
// 		[2, 2, 0]
// 		[2, 2, 1]
// 		[2, 2, 2]	
MultiDimArray.LoopByCoord(
	[](const SliceTestType::CoordinateType& InCoordinate,
			SliceTestType::IndexType InLinearIdx,
			SliceTestType::IndexType InLoopCount,
			SliceTestType::StorageDataType& InData)
		{
			// Use [InData] to access the data.

			// [InCoordinate] is the coordinate (like [0, 2, 1]) of the data.
			// [InLinearIdx] is the linear storage index of the data.
			// [InLoopCount] is the loop count (may not equal to [InLinearIdx] depending on the storage order).
		}
);
```
Recommend use `auto` keyword to replace `SliceTestType::XXXX` very long type name.
- InData: The type is same as the type that you multi-dimension array variable defined.
- InCoordinate: The type is `std::array<int, DIM_SIZE>`. `DIM_SIZE` is the number of dimensions.
- InLinearIdx: The type is `int`.
- InLoopCount: The type is `int`.

So, the above callback functions can be written as follows:
```cpp
// Loop by the linear storage index.
auto LoopFunc = [](auto InCoordinate, int InLinearIdx, int InLoopCount,
			YOUR_DATA_TYPE& InData)	{ ... }

```

#### Mask Operation
掩码操作是指通过自定义掩码数组（布尔值数组）来对多维数组（以下称作：数据数组）进行访问，旨在提供一个比切片操作更加自由精细的数据访问方式。掩码操作允许用户通过布尔值数组来指定哪些数据需要被访问，从而实现更加灵活的数据处理。  
Mask operations refer to accessing multi-dimensional arrays (hereafter referred to as data arrays) using a custom mask array (boolean array). The goal is to provide a more flexible and precise data access method compared to slicing operations.


本类中，掩码操作有以下特征：
In this class, mask operations have the following features:
- 掩码数组与需要访问数据数组**必须**具有**相同的维度**，但是大小可以随意。The mask array and the data array to be accessed **MUST** have **the SAME dimensions**, but the sizes can be arbitrary.
- 掩码数组中的每个元素都是一个`bool`或者是`int`，用于指示对应的数据数组元素是否需要被访问。Each element in the mask array is either a `bool` or an `int`, indicating whether the corresponding element in the data array should be accessed.
- 可以指定掩码数组的中心坐标以及掩码应用坐标。The center coordinate of the mask array and the apply coordinate of the mask can be specified.
- 掩码大小和应用坐标没有限制，且可以指定越界时的行为（即：数据数组为4x4大小，如果指定了(6, 7)作为应用坐标，则越界部分将如何处理）。There are no restrictions on the size of the mask and the apply coordinates, the out of range behavior can be specified (e.g., if the data array is 4x4 in size and (6, 7) is specified as the apply coordinate, how the out-of-range part is handled).
  - 不获取`EBorderMode::NoPadding`：越界部分不获取数据。
  - 循环模式`EBorderMode::RepeatBorder`：[1, 2, 3, 4] 则在该模式下看到的数组的样子是: Repeat mode: For [1, 2, 3, 4], the array appears as:

	| Index | -4 | -3 | -2 | -1 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
	|-------|----|----|----|----|---|---|---|---|---|---|---|---|---|---|----|----|
	| Data  | 1  | 2  | 3  | 4  | 1 | 2 | 3 | 4 | 1 | 2 | 3 | 4 | 1 | 2 | 3  | 4  |
  - 包含边界镜像模式`EBorderMode::ReflectBorder`：

	| Index | -4 | -3 | -2 | -1 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
	|-------|----|----|----|----|---|---|---|---|---|---|---|---|---|---|----|----|
	| Data  | 4  | 3  | 2  | 1  | 1 | 2 | 3 | 4 | 4 | 3 | 2 | 1 | 1 | 2 | 3  | 4  |
  - 不包含边界镜像模式`EBorderMode::Reflect101Border`

	| Index | -4 | -3 | -2 | -1 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
	|-------|----|----|----|----|---|---|---|---|---|---|---|---|---|---|----|----|
	| Data  | 3  | 4  | 3  | 2  | 1 | 2 | 3 | 4 | 3 | 2 | 1 | 2 | 3 | 4 | 3  | 2  |
- 返回值：返回一个一维数组，包含所有被掩码数组指示为`true`或`1`的数据（会包含重复项）。Return value: Returns a one-dimensional array containing all data indicated by `true` or `1` in the mask array (may include duplicates).
  - 顺序：通过掩码数组的`LoopByCoord`函数进行循环，回调中再根据`掩码元素坐标`、`中心坐标`以及`掩码应用坐标`计算出实际数据的坐标，所以返回值顺序取决于数据数组的内部存储顺序。Order: The order of the returned values depends on the internal storage order of the data array, as the mask array's `LoopByCoord` function is used to loop through the mask, and the actual data coordinates are calculated based on the mask element coordinates, center coordinates, and apply coordinates.

掩码操作示例代码：
Example code for mask operations:
```cpp
// Prepare the test data to get.
using SliceTestType = ArrayMultiDim::TArrayMultiDim<int, 10, 10>;
SliceTestType MultiDimTest_Slicing {};
auto ResetDataFunc = [](const ArrayMultiDim::TArrayMultiDim<int, 10, 10>::CoordinateType& InCoord, int InLinearIdx, int& InOldData) -> int
{
	return InLinearIdx + 1;  // Start from 1.
};
MultiDimTest_Slicing.SetData(ResetDataFunc);

// Make a mask data
// [MaskType] is the type-alias of the mask data.
// 		This type alias ensured the mask data has the same dimension as the data array, but the size is dynamic.
SliceTestType::MaskType MaskData {{
	{0, 1, 1, 0},
	{0, 1, 0, 1},
	{1, 0, 1, 1},
	{1, 0, 1, 0}
}};

// Get the data with the mask
TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, /* Apply Position: */{5, 5}, /* Mask Center Position: */{1, 1});
// 1st argument: The mask data.
// 2nd argument: The apply position of the mask data.
// 3rd argument: The center position of the mask data.
// The return value is the data array that the mask data indicates.

// For the above example, the return value will be:
// { 46, 47, 56, 58, 65, 67, 68, 75, 77 }

```

Details about the above example:

1. **Initialization of Data Array**:
   The data array `MultiDimTest_Slicing` is a 10x10 array initialized with values from 1 to 100 in a row-major order:
   ```
   00 01 02 03 04 05 06 07 08 09
   10 11 12 13 14 15 16 17 18 19
   20 21 22 23 24 25 26 27 28 29
   30 31 32 33 34 35 36 37 38 39
   40 41 42 43 44 45 46 47 48 49
   50 51 52 53 54 55 56 57 58 59
   60 61 62 63 64 65 66 67 68 69
   70 71 72 73 74 75 76 77 78 79
   80 81 82 83 84 85 86 87 88 89
   90 91 92 93 94 95 96 97 98 99
   ```

2. **Mask Array**:
   The mask array `MaskData` is a 4x4 array:
   ```
   0 1 1 0
   0 1 0 1
   1 0 1 1
   1 0 1 0
   ```

3. **Apply Position and Center Position**:
   The mask is applied to the data array with the apply position `{5, 5}` and the mask center position `{1, 1}`. This means the center of the mask (element at position `{1, 1}` in the mask) aligns with the element at position `{5, 5}` in the data array.

4. **Mask Apply**:
   The mask is applied to the data array starting from the top-left corner of the mask relative to the apply position. The coordinates in the data array that correspond to the mask are:
   ```
   (4, 4) (4, 5) (4, 6) (4, 7)
   (5, 4) (5, 5) (5, 6) (5, 7)
   (6, 4) (6, 5) (6, 6) (6, 7)
   (7, 4) (7, 5) (7, 6) (7, 7)
   ```

5. **Extracting Data**:
   Using the mask, we extract the values from the data array where the mask has `1`:
   - Mask position `{0, 1}` -> Data position `{4, 5}` -> Value `46`
   - Mask position `{0, 2}` -> Data position `{4, 6}` -> Value `47`
   - Mask position `{1, 1}` -> Data position `{5, 5}` -> Value `56`
   - Mask position `{1, 3}` -> Data position `{5, 7}` -> Value `58`
   - Mask position `{2, 0}` -> Data position `{6, 4}` -> Value `65`
   - Mask position `{2, 2}` -> Data position `{6, 6}` -> Value `67`
   - Mask position `{2, 3}` -> Data position `{6, 7}` -> Value `68`
   - Mask position `{3, 0}` -> Data position `{7, 4}` -> Value `75`
   - Mask position `{3, 2}` -> Data position `{7, 6}` -> Value `77`

6. **Return Value**:
   The extracted values are collected into a one-dimensional array in the order they were accessed, resulting in the return value:
   ```
   { 46, 47, 56, 58, 65, 67, 68, 75, 77 }
   ```
