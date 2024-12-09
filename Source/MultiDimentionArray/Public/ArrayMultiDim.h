#pragma once
#include "CoreMinimal.h"
#include <array>
#include <ranges>
#include <algorithm>
#include <type_traits>
#include <initializer_list>
#include <variant>

namespace ArrayMultiDim
{
	// 判断是否为 std::initializer_list 类型的 Concept
	template <typename T>
	concept IsNestedList = requires(T t)
	{
		typename std::decay_t<T>::value_type;
		requires std::is_same_v<T, std::initializer_list<typename std::decay_t<T>::value_type>>;
	};
#pragma region NestedList递归模板std::initializer_list
	// 定义递归模板，构造嵌套的 std::initializer_list 类型
	template <int Size, typename T = int>
	struct NestedListImpl
	{
		using Type = std::initializer_list<typename NestedListImpl<Size - 1, T>::Type>;
	};

	// 基础模板特化，递归结束条件
	template <typename T>
	struct NestedListImpl<1, T>
	{
		using Type = std::initializer_list<T>;
	};

	// 使用别名模板简化调用
	template <int Size, typename T = int>
	using NestList = typename NestedListImpl<Size, T>::Type;

#pragma endregion NestedList递归模板std::initializer_list

	// 用于指定存储顺序的模板
	// 例如 Odr<1, 0, 2> 表示按照第二维优先、第一维次之、第三维最低优先级的顺序存储
	// 第一维代表数据的最外层，第二维是中间层，第三维代表数据的最内层
	// 默认情况下：最内层的数据在内存中的地址是连续的，最外层的数据在内存中的地址是分散的
	// 所以默认的存储顺序写出来的话就是：Odr<2, 1, 0>
	template <int... _Vals>
	struct Odr
	{
	};

	/**
 * @brief Defines a dimension slice structure
 */
	struct FSlice
	{
		/**
		 * @brief Uses TVariant to store different types of slices.
		 */
		TVariant<int, TPair<int, int>, FEmptyVariantState> SliceInfo;

		/**
		 * @brief Constructor: single index.
		 * @param Index The index of the slice.
		 */
		FSlice(int Index) : SliceInfo(TInPlaceType<int>(), Index)
		{
		}

		/**
		 * @brief Constructor: range.
		 * @param Start The start index of the range.
		 * @param End The end index of the range.
		 */
		FSlice(int Start, int End) : SliceInfo(TInPlaceType<TPair<int, int>>(), TPair<int, int>{Start, End})
		{
		}

		/**
		 * @brief Constructor: all elements.
		 */
		FSlice() : SliceInfo(TInPlaceType<FEmptyVariantState>(), FEmptyVariantState())
		{
		}

		// Check if the slice is specified by a range (like NumPy [1:3]).
		bool IsRanged () const { return SliceInfo.IsType<TPair<int, int>>(); }

		// Check if the slice is specified by a single index (like NumPy [1]).
		bool IsSingle () const { return SliceInfo.IsType<int>(); }

		// Check if the slice is specified by all elements (like NumPy [:]).
		bool IsAllDim () const { return SliceInfo.IsType<FEmptyVariantState>(); }

		int GetSingle() const
		{
			check(IsSingle());
			return SliceInfo.Get<int>();
		}

		int GetRangeStart() const
		{
			check(IsRanged());
			return SliceInfo.Get<TPair<int, int>>().Key;
		}

		int GetRangeEnd() const
		{
			check(IsRanged());
			return SliceInfo.Get<TPair<int, int>>().Value;
		}

	};


	enum EResizeDataCopyPolicy
	{
		PreserveOldData,
		CoordinationCopy,
		SetToUninitializedValue,
		SetToInitialValue
	};


	// // Let data is [1, 2, 3, (4), 5], get data by MASK1 [0, {1}, 1, 0, 1],
	// // Mask center coordinate is 1 (the 2nd element of mask), apply at index 3
	// // Also let MASK2 [1, {1}, 1, 1, 1], Mask center and apply index are the same.
	// NoPadding,  // No padding, the border is not processed. result is [4, 5]; MASK2 result is [3, 4, 5]
	// RepeatBorder,  // Repeat from the start. result is [4, 5, 2]
	// ReflectBorder,  // Reflect from the border. result is [4, 5, 4]
	// Reflect101Border,  // Reflect from the border. but no border element duplicate. result is [4, 5, 3]
	// ConstantBorder  // Fill with a constant border value. result is [4, 5, 5]

	
	enum EBorderMode
	{
		// Let data is [1 2 3 4 5] No data represent by letter N
		// The range surround by [] is the original data, the outer is each border mode see.
		NoPadding,  // No padding, the border is not processed. result is [1 2 3 4 5] N N N N N
		RepeatBorder,  // Repeat from the start. result is [1 2 3 4 5] 1 2 3 4 5 1 2 3 4 5
		ReflectBorder,  // Reflect from the border. result is [1 2 3 4 5] 5 4 3 2 1 1 2 3 4 5
		Reflect101Border,  // Reflect from the border. but no border element duplicate. result is [1 2 3 4 5] 4 3 2 1 2 3 4 5 4 3 2 1
		ConstantBorder  // Fill with a constant border value. result is [1 2 3 4 5] 5 5 5 5 5 
	};

	

	template <typename DataType, int... Dims>
	class TArrayMultiDim
	{
		using DimSizeType = int;
		using IndexType = int;
		using SelfType = TArrayMultiDim<DataType, Dims...>;
		static constexpr int DYNAMIC_SIZE = -1;
		static constexpr int INVALID_INDEX = -1;

	public:
		using StorageDataType = DataType;
		using NestedListType = NestList<sizeof...(Dims), DataType>;
		using ArrayDimType = std::array<DimSizeType, sizeof...(Dims)>;
		using CoordinateType = std::array<IndexType, sizeof...(Dims)>;
		using MaskType = TArrayMultiDim<std::variant<bool, int>, (Dims > 0 ? DYNAMIC_SIZE : DYNAMIC_SIZE)...>;
		using SelfDynamicSizeType = TArrayMultiDim<DataType, (Dims > 0 ? DYNAMIC_SIZE : DYNAMIC_SIZE)...>;
		static constexpr DimSizeType DIM_SIZE = sizeof...(Dims);

	private:

		// 通用 constexpr 生成函数
		static constexpr ArrayDimType GenCompileTimeArray(int InSt = 0, int InStep = 0)
		{
			ArrayDimType RetArray{};
			for (int i = 0; i < DIM_SIZE; ++i)
			{
				RetArray[i] = InSt + i * InStep;
			}
			return RetArray;
		}

		static constexpr bool HasCompileTimeDynamicSize()
		{
			for (auto SingleDimSize : CompileTimeEachDimSize)
			{
				if (SingleDimSize == DYNAMIC_SIZE)
					return true;
			}
			return false;
		}

		static constexpr ArrayDimType CompileTimeEachDimSize = {Dims...};
		static constexpr std::array<bool, DIM_SIZE> CompileTimeDynamicDimFlagList = {(Dims == DYNAMIC_SIZE)...};

		CoordinateType TempIndexList = GenCompileTimeArray(INVALID_INDEX);
		// Just using in Initialize() function.
		ArrayDimType RuntimeEachDimSize = CompileTimeEachDimSize;
		CoordinateType RuntimeStorageOrder = GenCompileTimeArray(DIM_SIZE - 1, -1);
		// Default order is inner dimension first. Int sequence.
		// Stride values for each dimension, calculated based on the storage order
		CoordinateType RuntimeStride{
			[this]
			{
				if constexpr (HasCompileTimeDynamicSize())
				{
					return GenCompileTimeArray(INVALID_INDEX);
				}
				CoordinateType Strides;
				int CurrentStride = 1;
				for (size_t i = 0; i < RuntimeStorageOrder.size(); ++i)
				{
					Strides[RuntimeStorageOrder[i]] = CurrentStride;
					CurrentStride *= RuntimeEachDimSize[RuntimeStorageOrder[i]];
				}
				return Strides;
			}()
		};
		int TotalSize{
			[]
			{
				return HasCompileTimeDynamicSize() ? -1 : (Dims * ...);
			}()
		};

		TArray<DataType> DataList;


#pragma region 构造函数

	protected:
#pragma region HelperFuncs for Constructor

		static constexpr bool HasCompiledDynamicSize()
		{
			for (auto SingleDimSize : CompileTimeEachDimSize)
			{
				if (SingleDimSize == DYNAMIC_SIZE)
					return true;
			}
			return false;
		}	
		bool HasDynamicSize()
		{
			for (auto SingleDimSize : RuntimeEachDimSize)
			{
				if (SingleDimSize == DYNAMIC_SIZE)
					return true;
			}
			return false;
		}

		static bool HasDynamicSize(const ArrayDimType& InRuntimeEachDimSize,
								   std::vector<int>& OutDynamicSizeDimIndex,
								   int& OutTotalSize = []() -> int& {
									   static int defaultValue = 0; // 使用静态局部变量作为默认值
									   return defaultValue;
								   }())
		{
			OutDynamicSizeDimIndex.erase(OutDynamicSizeDimIndex.begin(), OutDynamicSizeDimIndex.end());
			int i = 0;
			OutTotalSize = 1;
			for (auto SingleDimSize : InRuntimeEachDimSize)
			{
				OutDynamicSizeDimIndex.push_back(i);
				if (SingleDimSize == DYNAMIC_SIZE)
				{
					OutTotalSize = DYNAMIC_SIZE;
					return true;
				}
				OutTotalSize *= SingleDimSize;
				i++;
			}
			return false;
		}

		/// 
		/// @param InRuntimeEachDimSize 
		/// @param OutTotalSize Optional, A int reference to output the total size, get -1 when [InRuntimeEachDimSize] has ANY -1 sized dimension.
		/// @return [InRuntimeEachDimSize] has ANY -1 sized dimension.
		static bool HasDynamicSize(const ArrayDimType& InRuntimeEachDimSize,
								   int& OutTotalSize = []() -> int& {
									   static int defaultValue = 0; // 使用静态局部变量作为默认值
									   return defaultValue;
								   }())
		{
			OutTotalSize = 1;
			for (auto SingleDimSize : InRuntimeEachDimSize)
			{
				if (SingleDimSize == DYNAMIC_SIZE)
				{
					OutTotalSize = DYNAMIC_SIZE;
					return true;
				}
				OutTotalSize *= SingleDimSize;
			}
			return false;
		}

		template <typename ElementType, int ArraySize, typename InvalidValType>
		static bool HasInvalidValue(const std::array<ElementType, ArraySize>& InArray,
									const InvalidValType& InInvalidVal)
		{
			for (auto ElementValue : InArray)
			{
				if (ElementValue == InInvalidVal)
				{
					return true;
				}
			}
			return false;
		}

		/**
		 * @brief Updates the stride values for each dimension based on the current storage order.
		 *
		 * This function calculates the stride values for each dimension of the multi-dimensional array.
		 * The stride value represents the number of elements to skip to move to the next element in the current dimension.
		 * It ensures that the size of each dimension is determined and not dynamic.
		 *
		 * Pre-requires:
		 *		1. The size of each dimension is determined and not dynamic.
		 */
		void UpdateStrides()
		{
			int CurrentStride = 1;
			for (size_t i = 0; i < RuntimeStorageOrder.size(); ++i)
			{
				checkf(RuntimeEachDimSize[i] != DYNAMIC_SIZE,
					   TEXT("Undetermined size of dimension is not allowed in this function."));
				RuntimeStride[RuntimeStorageOrder[i]] = CurrentStride;
				CurrentStride *= RuntimeEachDimSize[RuntimeStorageOrder[i]];
			}
		}

		// 获取嵌套初始化列表各个维度的大小
		template <int Level, typename ListType>
		void GetNestedListDimSize(const ListType& InList, ArrayDimType& InOutElementIndex)
		{
			int i = 0;
			InOutElementIndex[DIM_SIZE - Level] = InList.size();
				if constexpr (Level > 1)
				{
			for (const auto& Elem : InList)
			{
					GetNestedListDimSize<Level - 1>(Elem, InOutElementIndex);
			}
				}
		}

		/**
		 * @brief Validates the size of the initialization list is EQUAL to the compile-time dimension sizes.
		 *
		 * This function checks if the sizes of the initialization list match the expected compile-time sizes
		 * for each dimension. If there is a mismatch, it logs a warning message.
		 *
		 * @param InitListDimSize The sizes of the initialization list for each dimension.
		 */
		void ValidateInitListSize(const ArrayDimType& InitListDimSize)
		{
			for (int i = 0; i < DIM_SIZE; ++i)
			{
				check(InitListDimSize[i] == CompileTimeEachDimSize[i]);
				if (InitListDimSize[i] != CompileTimeEachDimSize[i])
				{
					UE_LOG(LogTemp, Warning,
						   TEXT(
							   "Function:[%hs] The input list size does not match the expected size, dimension index: [%d], expected size: [%d], input size: [%d]"
						   ), __FUNCTION__, i, CompileTimeEachDimSize[i], InitListDimSize[i]);
					return;
				}
			}
		}
		// template <int Level, typename ListType>
		// ArrayDimType GetNestedListDimSize(const ListType& InList)
		// {
		// 	ArrayDimType RetDimSize{};
		// 	auto TempList = InList;
		// 	for (int i = 0; i < DIM_SIZE; ++i)
		// 	{
		// 		RetDimSize[i] = TempList.size();
		// 		TempList = InList[0];
		// 	}
		// 	return RetDimSize;
		// }

		// 递归初始化函数
		template <int Level, typename ListType>
		void InitializeFromInputData(const ListType& InList, CoordinateType& InOutElementIndex)
		{
			int i = 0;
			for (const auto& Elem : InList)
			{
				InOutElementIndex[DIM_SIZE - Level] = i;
				if constexpr (Level > 1)
				{
					InitializeFromInputData<Level - 1>(Elem, InOutElementIndex);
				}
				else
				{
					IndexType InElementIndex = CoordinateToLinearIndex(InOutElementIndex, RuntimeStride);

					if constexpr (false)
					{
						FString ElementIndexStr = "";
						for (int j = 0; j < DIM_SIZE; j++)
						{
							ElementIndexStr += FString::FromInt(InOutElementIndex[j]) + TEXT(", ");
						}
						ElementIndexStr += TEXT(";");
						UE_LOG(LogTemp, Display, TEXT("%s%s%d"), *ElementIndexStr, *FString::ChrN(Level, '\t'), Elem);
					}

					DataList[InElementIndex] = Elem;
				}
				++i;
			}
			if constexpr (Level == DIM_SIZE - 1)
			{
				UpdateStrides();
			}
		}
#pragma endregion HelperFuncs for Constructor

	public:
#pragma region 无数据初始化构造
		TArrayMultiDim()
		{
		}

		template <int... IndexTypes>
		TArrayMultiDim(Odr<IndexTypes...>)
		{
			RuntimeStorageOrder = {IndexTypes...};
			UpdateStrides();
		}

		// Copy Constructor
		TArrayMultiDim(const TArrayMultiDim& InOther)
		{
			RuntimeEachDimSize = InOther.RuntimeEachDimSize;
			RuntimeStorageOrder = InOther.RuntimeStorageOrder;
			RuntimeStride = InOther.RuntimeStride;
			TotalSize = InOther.TotalSize;
			DataList = InOther.DataList;
		}
#pragma endregion 无数据初始化构造

#pragma region MultiDim Data Constructors
		explicit TArrayMultiDim(const NestedListType& InList)
		{
			ArrayDimType InitListDimSize;
			GetNestedListDimSize<DIM_SIZE>(InList, InitListDimSize);
			// for (int i = 0; i < DIM_SIZE; ++i)
			// {
			// 	UE_LOG(LogTemp, Display, TEXT("Function:[%hs] Size: %d"), __FUNCTION__, InitListDimSize[i]);
			// }
			if constexpr (HasCompiledDynamicSize())
			{
				RuntimeEachDimSize = InitListDimSize;
				// Has dynamic size, set the runtime size and stride to the initial list size.
				SetDimSize(InitListDimSize);
			}
			else
			{
				// No dynamic size, validate the size of the initialization list should same as the compile-time size.
				ValidateInitListSize(InitListDimSize);
			}
			// Normal storage order, no need [UpdateStrides()]
			DataList.SetNumUninitialized(TotalSize);
			InitializeFromInputData<DIM_SIZE>(InList, TempIndexList);
		}

		template <int... IndexTypes>
		explicit TArrayMultiDim(const NestedListType& InList, Odr<IndexTypes...>)
		{
			ArrayDimType InitListDimSize;
			GetNestedListDimSize<DIM_SIZE>(InList, InitListDimSize);
			// for (int i = 0; i < DIM_SIZE; ++i)
			// {
			// 	UE_LOG(LogTemp, Display, TEXT("Function:[%hs] Size: %d"), __FUNCTION__, InitListDimSize[i]);
			// }
			if constexpr (HasCompiledDynamicSize())
			{
				RuntimeEachDimSize = InitListDimSize;
				// Has dynamic size, set the runtime size and stride to the initial list size.
				SetDimSize(InitListDimSize);
			}
			else
			{
				// No dynamic size, validate the size of the initialization list should same as the compile-time size.
				ValidateInitListSize(InitListDimSize);
			}
			RuntimeStorageOrder = {IndexTypes...};
			UpdateStrides();
			DataList.SetNumUninitialized(TotalSize);
			InitializeFromInputData<DIM_SIZE>(InList, TempIndexList);
		}

#pragma endregion MultiDim Data Constructors

#pragma endregion 构造函数

#pragma region SetDimSize

	protected:

		void UpdateTotalSize()
		{
			TotalSize = 1;
			for (int i = 0; i < DIM_SIZE; ++i)
			{
				TotalSize *= RuntimeEachDimSize[i];
			}
		}

		// 将多维坐标转换为线性序号
		static IndexType CoordinateToLinearIndex(const CoordinateType& InCoordinate,
												 const CoordinateType& InStride)
		{
			IndexType RetLinearIndex = 0;
			for (int i = 0; i < DIM_SIZE; ++i)
			{
				RetLinearIndex += InStride[i] * InCoordinate[i];
			}
			return RetLinearIndex;
		}

		// 将线性序号转换为多维坐标
		static CoordinateType IndexToCoordinate(int InIndex,
																 const CoordinateType& InStride,
																 const CoordinateType& InStorageOrder)
		{
			CoordinateType Coordinates;
			for (size_t i = 0; i < DIM_SIZE; ++i)
			{
				const int& Dim = InStorageOrder[DIM_SIZE - i - 1];
				Coordinates[Dim] = InIndex / InStride[Dim];
				InIndex %= InStride[Dim];
			}
			return Coordinates;
		}

		static bool IsCoordinateOversize(const CoordinateType& InCoordinate,
										 const ArrayDimType& InEachDimSize)
		{
			for (int i = 0; i < DIM_SIZE; ++i)
			{
				if (InCoordinate[i] >= InEachDimSize[i] or InCoordinate[i] < 0)
					return true;
			}
			return false;
		}

		void CoordinationCopyData_Internal(const TArray<DataType>& InOldDataList, TArray<DataType>& InNewDataList,
										   const ArrayDimType& OldRuntimeEachDimSize,
										   const CoordinateType& OldRuntimeStride,
										   const CoordinateType& InOldStorageOrder,
										   const ArrayDimType& NewRuntimeEachDimSize,
										   const CoordinateType& NewRuntimeStride)
		{
			DimSizeType OldDataSize, NewDataSize;
			HasDynamicSize(OldRuntimeEachDimSize, OldDataSize);
			HasDynamicSize(NewRuntimeEachDimSize, NewDataSize);
			check(OldDataSize == InOldDataList.Num());

			if (InNewDataList.Num() != NewDataSize)
			{
				InNewDataList.SetNumUninitialized(NewDataSize);
			}

			for (int i = 0; i < InOldDataList.Num(); ++i)
			{
				auto Coord = IndexToCoordinate(i, OldRuntimeStride, InOldStorageOrder);
				if (IsCoordinateOversize(Coord, NewRuntimeEachDimSize))
				{
					continue;
				}

				IndexType NewIndex = CoordinateToLinearIndex(Coord, NewRuntimeStride);
				InNewDataList[NewIndex] = InOldDataList[i];
			}
		}

	public:
		/**
		 * @brief Sets the size of each dimension of the multi-dimensional array.
		 * 
		 * This function sets the size of each dimension of the multi-dimensional array. It checks if the provided sizes match the expected dimension size (DIM_SIZE).
		 * If the new sizes do not contain any dynamic sizes, it updates the runtime sizes, strides, and copies the old data to the new data array.
		 * If the new sizes contain dynamic sizes, it logs a warning message.
		 * 
		 * @param InSize An array representing the sizes of each dimension.
		 * @param InNewOrder The new storage order of the dimensions.
		 */
		void SetDimSize(const ArrayDimType& InSize,
						const CoordinateType& InNewOrder,
						EResizeDataCopyPolicy InCopyPolicy = EResizeDataCopyPolicy::CoordinationCopy)
		{
			const ArrayDimType& NewRuntimeEachDimSize = InSize;
			const CoordinateType OldStorageOrder = RuntimeStorageOrder;
			RuntimeStorageOrder = InNewOrder;
			int NewTotalSize = DYNAMIC_SIZE;

			// 检查是否有未知大小的维度
			std::vector<int> DynamicDimIndexList;
			// 有的话，直接返回
			if (HasDynamicSize(NewRuntimeEachDimSize, DynamicDimIndexList, NewTotalSize))
			// Has undetermined dynamic size.
			{
				// 将 std::vector 转换为 TArray
				TArray<FString> StringArray;
				Algo::Transform(DynamicDimIndexList, StringArray, [](int32 Number)
				{
					return FString::FromInt(Number);
				});
				// 使用 FString::Join
				FString DynamicDimIndexStr = FString::Join(StringArray, TEXT(", "));
				UE_LOG(LogTemp, Warning,
					   TEXT(
						   "Function:[%hs] This multi-dimension array HAS UNKNOWN sized dimension, dimension index: [%s]"
					   ), __FUNCTION__, *DynamicDimIndexStr);
				return;
			}

			// Store the old [EachDimSize] and [Stride] values
			ArrayDimType OldRuntimeEachDimSize = RuntimeEachDimSize;
			CoordinateType OldRuntimeStride = RuntimeStride;

			// Update the runtime sizes, total size, and strides, also initialize the [DataList] with the new total size.
			RuntimeEachDimSize = NewRuntimeEachDimSize;
			UpdateTotalSize();
			UpdateStrides();

			// Resize the data list based on the new total size and the copy policy
			if (InCopyPolicy == EResizeDataCopyPolicy::PreserveOldData)
			{
				DataList.SetNum(NewTotalSize);
			}
			else if (InCopyPolicy == EResizeDataCopyPolicy::SetToInitialValue)
			{
				DataList.SetNumZeroed(NewTotalSize);
			}
			else if (InCopyPolicy == EResizeDataCopyPolicy::SetToUninitializedValue)
			{
				DataList.SetNumUninitialized(NewTotalSize);
			}
			else if (InCopyPolicy == EResizeDataCopyPolicy::CoordinationCopy)
			{
				bool bHasNoneOldData = HasInvalidValue(OldRuntimeEachDimSize, DYNAMIC_SIZE) || HasInvalidValue(OldRuntimeStride, DYNAMIC_SIZE) || DataList.IsEmpty();
				if (!bHasNoneOldData)
				{
					TArray<DataType> NewData;
					CoordinationCopyData_Internal(DataList, NewData, OldRuntimeEachDimSize, OldRuntimeStride,
												  OldStorageOrder, NewRuntimeEachDimSize, RuntimeStride);
					DataList = MoveTemp(NewData);
				}
			}
		}

		void SetDimSize(const ArrayDimType& InSize,
						EResizeDataCopyPolicy InCopyPolicy = EResizeDataCopyPolicy::CoordinationCopy)
		{
			SetDimSize(InSize, RuntimeStorageOrder, InCopyPolicy);
		}

		template <int... OrderList>
		void SetDimSize(const ArrayDimType& InSize, Odr<OrderList...>,
						EResizeDataCopyPolicy InCopyPolicy = EResizeDataCopyPolicy::CoordinationCopy)
		{
			SetDimSize(InSize, {OrderList...}, InCopyPolicy);
		}

		void SetData(const NestedListType& InDataList)
		{
			InitializeFromInputData<DIM_SIZE>(InDataList, TempIndexList);
		}

		/**
		 * \brief Type definition for a function that initializes data elements.
		 *
		 * This function type is used to initialize data elements in the multi-dimensional array.
		 * It takes the coordinate of the element, the linear index of the element, and the old data value as parameters.
		 *
		 * \param InCoordinate The coordinate of the element in the multi-dimensional array, like {0, 0, 0}.
		 * \param InLinearIdx The linear index of the element in the data list, like 0. This number is calculated based on the storage order.
		 *						But in the SetData function, it loops over the data list (0 ~ TotalSize) in the linear order.
		 * \param InOldData The old data value of the element.
		 * \return The new data value for the element.
		 */
		using DataInitializerFuncType = std::function<DataType(
			const CoordinateType& /* InCoordinate */,
			IndexType /* InLinearIdx */,
			DataType& /* InOldData */)>;

		/**
		 * \brief Sets the data of the multi-dimensional array using a custom initializer function.
		 * Initial order is the linear order of the data list ( means the callback function's 
		 *
		 * This function iterates over each element in the multi-dimensional array and sets its value
		 * using the provided initializer function. The initializer function takes the coordinate of the element,
		 * the linear index of the element, and the old data value as parameters.
		 *
		 * \param InFunc The initializer function to set the data elements.
		 */
		void SetData(DataInitializerFuncType InFunc)
		{
			if (DataList.Num() != TotalSize)
			{
				DataList.SetNum(TotalSize);
			}
			for (int i = 0; i < TotalSize; ++i)
			{
				auto Coord = IndexToCoordinate(i, RuntimeStride, RuntimeStorageOrder);
				DataList[i] = InFunc(Coord, i, DataList[i]);
			}
		}

#pragma endregion SetDimSize

#pragma region GetElements

#pragma region HelperFunctions
		using NestedLoopCallbackType = std::function<void(const CoordinateType& /* InLoopIndex */, const int& /* InLoopCounter */)>;

		static void DoNestedLoops(const CoordinateType& Limits, const NestedLoopCallbackType& InFunc)
		{
			CoordinateType Indices{};  // Zero-initialize all elements
			int Counter = 0;

			while (true)
			{
				// Execute callback with current indices
				InFunc(Indices, Counter++);

				// Increment indices from last dimension
				int Level = DIM_SIZE - 1;
				while (Level >= 0 && ++Indices[Level] == Limits[Level])
				{
					Indices[Level--] = 0;  // Reset current level and carry over
				}

				// Exit if outermost level overflows
				if (Level < 0)
				{
					break;
				}
			}
		}
#pragma endregion HelperFunctions 

	public:
		DataType& operator[](const IndexType& InElementLinearIndex)
		{
			return DataList[InElementLinearIndex];
		}

		template <typename... T>
		DataType& operator()(T... InElementCoordinate)
		{
			const IndexType ElementIndex = CoordinateToLinearIndex({InElementCoordinate...}, RuntimeStride);
			return DataList[ElementIndex];
		}

		// Begin and end methods for non-const iterators
		typename TArray<DataType>::TIterator CreateIterator() { return DataList.CreateIterator(); }

		// Begin and end methods for const iterators
		typename TArray<DataType>::TConstIterator CreateConstIterator() const { return DataList.CreateConstIterator(); }

		/**
		 * \brief Loop all elements by the linear storage index.
		 *
		 * Lambda function example:
		 * \code 
		 *		[](const CoordinateType& InCoordinate, IndexType InLinearIdx, DataType& InData) -> DataType
		 *		{ }
		 * \endcode 
		 *
		 * \param InCoordinate The coordinate of the element, like {0, 0, 0}.
		 * \param InLinearIdx The linear index of the element in the data list, like 0.
		 * \param InData The data value reference of the element.
		 * \return The new data value for the element.
		 */
		using LoopCallbackType = std::function<void(
			const CoordinateType& /* InCoordinate */,
			IndexType /* InLinearIdx */,
			IndexType /* InLoopCount */,
			DataType& /* InData */)>;
		using ConstLoopCallbackType = std::function<void(
			const CoordinateType& /* InCoordinate */,
			IndexType /* InLinearIdx */,
			IndexType /* InLoopCount */,
			const DataType& /* InData */)>;
		void LoopByIndex(LoopCallbackType InFunc, const bool& InCalcCoord = false)
		{
			static CoordinateType NoneCoord;
			if (InCalcCoord)
			{
				for (int i = 0; i < TotalSize; ++i)
				{
					auto Coord = IndexToCoordinate(i, RuntimeStride, RuntimeStorageOrder);
					InFunc(Coord, i, i, DataList[i]);
				}
			}
			else
			{
				for (int i = 0; i < TotalSize; ++i)
				{
					InFunc(NoneCoord, i, i, DataList[i]);
				}
			}
		}
		void ConstLoopByIndex(ConstLoopCallbackType InFunc, const bool& InCalcCoord = false) const
		{
			static CoordinateType NoneCoord;
			if (InCalcCoord)
			{
				for (int i = 0; i < TotalSize; ++i)
				{
					auto Coord = IndexToCoordinate(i, RuntimeStride, RuntimeStorageOrder);
					InFunc(Coord, i, i, DataList[i]);
				}
			}
			else
			{
				for (int i = 0; i < TotalSize; ++i)
				{
					InFunc(NoneCoord, i, i, DataList[i]);
				}
			}
		}
		void LoopByCoord(const LoopCallbackType& InFunc)
		{
			DoNestedLoops(RuntimeEachDimSize, [&](const CoordinateType& InLoopIndex, const int& InLoopCounter)
			{
				IndexType LinearIndex = CoordinateToLinearIndex(InLoopIndex, RuntimeStride);
				InFunc(InLoopIndex, LinearIndex, InLoopCounter, DataList[LinearIndex]);
			});
		}
		void ConstLoopByCoord(const ConstLoopCallbackType& InFunc) const
		{
			DoNestedLoops(RuntimeEachDimSize, [&](const CoordinateType& InLoopIndex, const int& InLoopCounter)
			{
				IndexType LinearIndex = CoordinateToLinearIndex(InLoopIndex, RuntimeStride);
				InFunc(InLoopIndex, LinearIndex, InLoopCounter, DataList[LinearIndex]);
			});
		}

#pragma endregion GetElements

	public:
		// Getter for the total size.
		int GetTotalSize() const { return TotalSize; }

		// Getter for the [RuntimeStride] values.
		const CoordinateType& GetRuntimeStride() const { return RuntimeStride; }

#pragma region SlicingOperator

	public:
		// 切片操作函数
		SelfDynamicSizeType Slice(std::initializer_list<FSlice> InSlices) const
		{
			// 1. 计算新维度大小
			ArrayDimType NewDimensions;
			int i = 0;
			for (auto Slice : InSlices)
			{
				if (Slice.SliceInfo.IsType<int>())
				{
					// 单个索引，该维度大小为1
					NewDimensions[i] = 1;
				}
				else if (Slice.SliceInfo.IsType<TPair<int, int>>())
				{
					// 范围切片，计算范围大小
					const auto& Range = Slice.SliceInfo.Get<TPair<int, int>>();
					NewDimensions[i] = Range.Value - Range.Key;
				}
				else
				{
					// 全部元素
					NewDimensions[i] = RuntimeEachDimSize[i];
				}
				i++;
			}

			// 2. 创建新数组
			SelfDynamicSizeType Result;
			Result.SetDimSize(NewDimensions, EResizeDataCopyPolicy::SetToUninitializedValue);

			// 3. 填充数据
			CoordinateType CurrentCoord;
			FillSlicedData(Result, InSlices, CurrentCoord, 0);

			return Result;
		}

	private:
		// 递归填充切片数据
		void FillSlicedData(SelfDynamicSizeType& OutResult,
							const std::initializer_list<FSlice>& InSlices,
							CoordinateType& InOutCoord,
							int InDimIndex) const
		{
			if (InDimIndex == DIM_SIZE)
			{
				// Enter the last dimension, start to fill the data.
				IndexType SrcIndex = CoordinateToLinearIndex(InOutCoord, RuntimeStride);

				// Adjust the InOutCoord by subtracting the slice start coordinate.
				CoordinateType AdjustedCoord = InOutCoord;
				// for (int j = 0; j < DIM_SIZE; ++j)
				int j = 0;
				for (auto SliceObj : InSlices)
				{
					// Adjust the coordinate if the slice is a range.
					if (SliceObj.IsRanged())
					{
						AdjustedCoord[j] -= SliceObj.GetRangeStart();
					}
					else if (SliceObj.IsSingle())
					{
						AdjustedCoord[j] -= SliceObj.GetSingle();
					}
					j++;
				}
				IndexType DstIndex = CoordinateToLinearIndex(AdjustedCoord, OutResult.GetRuntimeStride());
				OutResult[DstIndex] = DataList[SrcIndex];  // OutResult[DstIndex] use the public operator[] function.
				return;
			}

			// auto SlicesArray = InSlices.begin();
			const FSlice& Slice = *(InSlices.begin() + InDimIndex);
			// const auto& Slice = InSlices[InDimIndex];
			if (Slice.IsSingle())
			{
				// Single index
				InOutCoord[InDimIndex] = Slice.GetSingle();
				FillSlicedData(OutResult, InSlices, InOutCoord, InDimIndex + 1);
			}
			else if (Slice.IsRanged())
			{
				// A range of indexes.
				for (int i = Slice.GetRangeStart(); i < Slice.GetRangeEnd(); ++i)
				{
					InOutCoord[InDimIndex] = i;
					FillSlicedData(OutResult, InSlices, InOutCoord, InDimIndex + 1);
				}
			}
			else if (Slice.IsAllDim())
			{
				// All elements in the dimension.
				for (int i = 0; i < RuntimeEachDimSize[InDimIndex]; ++i)
				{
					InOutCoord[InDimIndex] = i;
					FillSlicedData(OutResult, InSlices, InOutCoord, InDimIndex + 1);
				}
			}
		}
#pragma endregion SlicingOperator

	public:
		CoordinateType GetRuntimeStorageOrder() const { return RuntimeStorageOrder; }

#pragma region MaskDataGetter
		TArray<DataType> GetElementsByMask(const MaskType& InMask,
										   const CoordinateType& InApplyCoord,
										   const CoordinateType& InMaskCenter = GenCompileTimeArray(0),
										   EBorderMode InBorderMode = EBorderMode::NoPadding) const
		{
			TArray<DataType> Result;
			Result.Reserve(InMask.GetTotalSize());  // Pre-allocate the result array.

			// Iterate over the mask
			InMask.ConstLoopByCoord([&](const CoordinateType& MaskCoord, IndexType MaskLinearIdx, IndexType MaskLoopCount, const std::variant<bool, int>& MaskValue)
			{
				// Access the mask value whatever the type is bool or int. The int value will be converted to bool
				//		with the following rules: 0 -> false, None-Zeroed values -> true.
				bool bEnabledData = std::visit([]<typename T0>(T0&& InMaskValue) -> bool
					{
						using T = std::decay_t<T0>;
						if constexpr (std::is_same_v<T, bool>)
						{
							return InMaskValue;
						}
						else if constexpr (std::is_same_v<T, int>)
						{
							return static_cast<bool>(InMaskValue);
						}
						return false;
					},
				MaskValue);
				if (!bEnabledData)
				{
					return;
				}
				
				// Calculate the corresponding coordinate in the original array
				CoordinateType OriginalCoord;
				for (int i = 0; i < DIM_SIZE; ++i)
				{
					OriginalCoord[i] = InApplyCoord[i] + MaskCoord[i] - InMaskCenter[i];
				}
			
				if (!IsCoordinateOversize(OriginalCoord, RuntimeEachDimSize))
				{
					// Add the element to the result
					IndexType OriginalLinearIdx = CoordinateToLinearIndex(OriginalCoord, RuntimeStride);
					Result.Add(DataList[OriginalLinearIdx]);
					return;
				}

				// Handle [out of range] conditions
				switch (InBorderMode)
				{
				case EBorderMode::NoPadding:  // No padding, skip the element.
					return;
				case EBorderMode::RepeatBorder:
					for (int i = 0; i < DIM_SIZE; ++i)
					{
						OriginalCoord[i] = (OriginalCoord[i] % RuntimeEachDimSize[i] + RuntimeEachDimSize[i]) % RuntimeEachDimSize[i];
					}
					break;
				case EBorderMode::ReflectBorder:
					for (int i = 0; i < DIM_SIZE; ++i)
					{
						if (OriginalCoord[i] < 0)
						{
							OriginalCoord[i] = -OriginalCoord[i] - 1;  //  -(-11) - 1 = 10
						}
						if (OriginalCoord[i] >= RuntimeEachDimSize[i])
						{
							int period = 2 * RuntimeEachDimSize[i];
							OriginalCoord[i] = OriginalCoord[i] % period;  // 10 % 20 = 10
							if (OriginalCoord[i] >= RuntimeEachDimSize[i]) // 10 >= 10
							{
								OriginalCoord[i] = period - OriginalCoord[i] - 1;  // 20 - 11 - 1 = 8
							}
						}
					}
					break;
				case EBorderMode::Reflect101Border:
					for (int i = 0; i < DIM_SIZE; ++i)
					{
						if (OriginalCoord[i] < 0)
						{
							OriginalCoord[i] = -OriginalCoord[i];
						}
						if (OriginalCoord[i] >= RuntimeEachDimSize[i])  // 16 >= 10
						{
							int period = 2 * RuntimeEachDimSize[i] - 2;  // 20 - 2 = 18
							OriginalCoord[i] = OriginalCoord[i] % period;  // 16 % 18 = 16
							if (OriginalCoord[i] >= RuntimeEachDimSize[i])  // 16 >= 10
							{
								OriginalCoord[i] = period - OriginalCoord[i];  // 18 - 16 = 2
							}
						}
					}
					break;
				case EBorderMode::ConstantBorder:  // Not implement yet， Add the constant border value.
					// Result.Add(ConstantBorderValue);
					// Result.AddZeroed();
					return;
				}
			
				// Add the element to the result
				IndexType OriginalLinearIdx = CoordinateToLinearIndex(OriginalCoord, RuntimeStride);
				Result.Add(DataList[OriginalLinearIdx]);
			});

			return Result;
		}
#pragma endregion MaskDataGetter
	};  // Class TArrayMultiDim END
}
