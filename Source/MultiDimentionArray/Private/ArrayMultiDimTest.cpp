#pragma once
#include "Misc/AutomationTest.h"
#include "ArrayMultiDim.h"


template<typename ArrayMultiType>
bool LogAndValidateData(FAutomationTestBase* InTestObjPtr, const ArrayMultiType& MultiDimTest_Order, const TArray<int>& ExpectedOrder021)
{
	UE_LOG(LogTemp, Display, TEXT("Function:[%hs] =========================== RuntimeStorageOrder: %s"), __FUNCTION__, *FString::JoinBy(MultiDimTest_Order.GetRuntimeStorageOrder(), TEXT(", "), [](int32 Order) { return FString::FromInt(Order); }));
	for (auto It = MultiDimTest_Order.CreateConstIterator(); It; ++It)
	{
		auto& data = *It;
		// UE_LOG(LogTemp, Display, TEXT("Function:[%hs] Data: %d"), __FUNCTION__, data);
	}

	int LoopIndex = 0;
	for (auto It = MultiDimTest_Order.CreateConstIterator(); It; ++It)
	{
		if (!InTestObjPtr->TestEqual(TEXT("Data mismatch"), *It, ExpectedOrder021[LoopIndex]))
		{
			return false;
		}
		LoopIndex++;
	}
	return true;
}

using NestedLoopCallbackType = std::function<void(const TArray<int>& /* InLoopIndex */)>;
void DoNestedLoops(const TArray<int>& Limits, const NestedLoopCallbackType& InFunc)
{
	TArray<int> Indices;
	Indices.Init(0, Limits.Num()); // 初始化每层索引为 0

	while (true)
	{
		// 清空 Combination
		InFunc(Indices);

		// 从最后一层递增索引并处理进位
		int Level = Limits.Num() - 1;
		while (Level >= 0 && ++Indices[Level] == Limits[Level])
		{
			Indices[Level--] = 0; // 当前层溢出，重置并进位
		}

		// 如果最外层也溢出，退出
		if (Level < 0)
		{
			break;
		}
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(ArrayMultiDimTest, "ArrayMultiDim.ArrayMultiDimTest",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool ArrayMultiDimTest::RunTest(const FString& Parameters)
{
	TArray<int> ExpectedOrder012 = {
		1, 10, 19, 4, 13, 22, 7, 16, 25,
		2, 11, 20, 5, 14, 23, 8, 17, 26,
		3, 12, 21, 6, 15, 24, 9, 18, 27
	};TArray<int> ExpectedOrder021 = {
		1, 10, 19, 2, 11, 20, 3, 12, 21,
		4, 13, 22, 5, 14, 23, 6, 15, 24,
		7, 16, 25, 8, 17, 26, 9, 18, 27
	};TArray<int> ExpectedOrder102 = {
		1, 4, 7, 10, 13, 16, 19, 22, 25,
		2, 5, 8, 11, 14, 17, 20, 23, 26,
		3, 6, 9, 12, 15, 18, 21, 24, 27
	};TArray<int> ExpectedOrder120 = {
		1, 4, 7, 2, 5, 8, 3, 6, 9,
		10, 13, 16, 11, 14, 17, 12, 15, 18,
		19, 22, 25, 20, 23, 26, 21, 24, 27
	};TArray<int> ExpectedOrder201 = {
		1, 2, 3, 10, 11, 12, 19, 20, 21,
		4, 5, 6, 13, 14, 15, 22, 23, 24,
		7, 8, 9, 16, 17, 18, 25, 26, 27
	};TArray<int> ExpectedOrder210 = {
		1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24, 25, 26, 27
	};
	
	// This block tests the multi-dimensional array with a specific order
	{
		PushContext("Fixed dimension size with specific order");
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

		ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimTest_Order_012 {TestDataList, ArrayMultiDim::Odr<0, 1, 2>()};
		TestTrue("ExpectedOrder012", LogAndValidateData(this, MultiDimTest_Order_012, ExpectedOrder012));

		ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimTest_Order_021 {TestDataList, ArrayMultiDim::Odr<0, 2, 1>()};
		TestTrue("ExpectedOrder021", LogAndValidateData(this, MultiDimTest_Order_021, ExpectedOrder021));

		ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimTest_Order_102 {TestDataList, ArrayMultiDim::Odr<1, 0, 2>()};
		TestTrue("ExpectedOrder102", LogAndValidateData(this, MultiDimTest_Order_102, ExpectedOrder102));

		ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimTest_Order_120 {TestDataList, ArrayMultiDim::Odr<1, 2, 0>()};
		TestTrue("ExpectedOrder120", LogAndValidateData(this, MultiDimTest_Order_120, ExpectedOrder120));

		ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimTest_Order_201 {TestDataList, ArrayMultiDim::Odr<2, 0, 1>()};
		TestTrue("ExpectedOrder201", LogAndValidateData(this, MultiDimTest_Order_201, ExpectedOrder201));

		ArrayMultiDim::TArrayMultiDim<int, 3,3,3> MultiDimTest_Order_210 {TestDataList, ArrayMultiDim::Odr<2, 1, 0>()};
		TestTrue("ExpectedOrder210", LogAndValidateData(this, MultiDimTest_Order_210, ExpectedOrder210));

		PopContext();
	}

	// This block tests the multi-dimensional array with a dynamic dimension size.
	{
		PushContext("Dynamic dimension size");
		ArrayMultiDim::TArrayMultiDim<int, -1, -1, -1> FullDynamicArray;
		using TestArrayType = decltype(FullDynamicArray);
		auto ResetDataFunc = [](const TestArrayType::CoordinateType& InCoord, int InLinearIdx, int& InOldData) -> int
		{
			// UE_LOG(LogTemp, Display, TEXT("Function:[%hs] coord: %s"), __FUNCTION__, *FString::JoinBy(InCoord, TEXT(", "), [](int32 Coord) { return FString::FromInt(Coord); }));
			return InCoord[0] * 9 + InCoord[1] * 3 + InCoord[2] + 1;
			// return InLinearIdx + 1;
		};
		
		FullDynamicArray.SetDimSize({3, 3, 3});
		FullDynamicArray.SetData(ResetDataFunc);
		TestTrue("ExpectedOrder210", LogAndValidateData(this, FullDynamicArray, ExpectedOrder210));
		TestTrue("ExpectedOrder210 Data check: ", FullDynamicArray(2, 0, 1) == 20);

		TArray<TTuple<std::array<int, 3>, TArray<int>>> ExpectedOrders {
			{ std::array<int, 3>{2, 0, 1}, ExpectedOrder201 },
			{ std::array<int, 3>{2, 1, 0}, ExpectedOrder210 },
			{ std::array<int, 3>{1, 0, 2}, ExpectedOrder102 },
			{ std::array<int, 3>{1, 2, 0}, ExpectedOrder120 },
			{ std::array<int, 3>{0, 1, 2}, ExpectedOrder012 },
			{ std::array<int, 3>{0, 2, 1}, ExpectedOrder021 }
		};
		for (auto It = ExpectedOrders.CreateConstIterator(); It; ++It)
		{
			const std::array<int, 3> Order = ExpectedOrders[It.GetIndex()].Get<0>();
			const auto ExpectedOrder = ExpectedOrders[It.GetIndex()].Get<1>();
			FString OrderStr = FString::Printf(TEXT("Storage Order [%c, %c, %c]"), Order[0] + '0', Order[1] + '0', Order[2] + '0');
			FString Msg = FString::Printf(TEXT("%s Data check: "), *OrderStr);
			
		    FullDynamicArray.SetDimSize({3, 3, 3}, Order);
		    TestTrue(OrderStr, LogAndValidateData(this, FullDynamicArray, ExpectedOrder));

			// Test all storage data can be accessed correctly by the coordinate. 
			int LoopIndex = 1; 
			for (int i = 0; i < 3; ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					for (int k = 0; k < 3; ++k)
					{
						TestTrue(Msg, FullDynamicArray(i, j, k) == LoopIndex);
						LoopIndex++;
					}
				}
			}
		}

		PopContext();
	}

	// This block tests the dynamic size when change the size of dimension
	{
		ArrayMultiDim::TArrayMultiDim<int, -1, -1, -1> FullDynamicArray;
		using TestArrayType = decltype(FullDynamicArray);
		auto ResetDataFunc = [](const TestArrayType::CoordinateType& InCoord, int InLinearIdx, int& InOldData) -> int
		{
			return InCoord[0] * 9 + InCoord[1] * 3 + InCoord[2] + 1;
		};

		// Initialize the array with a size of 3x3x3 and order 0, 1, 2
		PushContext("Initialize the array with a size of 3x3x3 and order 0, 1, 2");
		FullDynamicArray.SetDimSize({3, 3, 3}, {0, 1, 2});
		FullDynamicArray.SetData(ResetDataFunc);
		TestTrue("ExpectedOrder012", LogAndValidateData(this, FullDynamicArray, ExpectedOrder012));
		TestArrayType OriginData = FullDynamicArray;
		PopContext();

		// Change the size of the dimension to 5x4x7, and the order to 1, 0, 2; The data copy policy is [CoordinationCopy]
		PushContext("Change the size of the dimension to 5x4x7, and the order to 1, 0, 2; The data copy policy is [CoordinationCopy]");
		FullDynamicArray.SetDimSize({5, 4, 7}, {1, 0, 2}, ArrayMultiDim::EResizeDataCopyPolicy::CoordinationCopy);
		for (int i = 0; i < 3; ++i)
		{
		    for (int j = 0; j < 3; ++j)
		    {
		        for (int k = 0; k < 3; ++k)
		        {
		            TestTrue("size of 3x3x3 2, 1, 0 -> 5x4x7 1, 0, 2: ", FullDynamicArray(i, j, k) == OriginData(i, j, k));
		        }
		    }
		}
		PopContext();

		// Change the size of the dimension to 2x1x2, and the order to 1, 2, 0; The data copy policy is [CoordinationCopy]
		PushContext("Change the size of the dimension to 2x1x2, and the order to 1, 2, 0; The data copy policy is [CoordinationCopy]");
		FullDynamicArray.SetDimSize({2, 1, 2}, {1, 2, 0}, ArrayMultiDim::EResizeDataCopyPolicy::CoordinationCopy);
		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 1; ++j)
			{
				for (int k = 0; k < 2; ++k)
				{
					TestTrue("size of 5x4x7 1, 0, 2 -> 2x1x2 1, 2, 0: ", FullDynamicArray(i, j, k) == OriginData(i, j, k));
				}
			}
		}
		PopContext();
	}

	{
		// Slicing operator testing
		PushContext("Slicing operator testing");
		using SliceTestType = ArrayMultiDim::TArrayMultiDim<int, 10, 10>;
		SliceTestType MultiDimTest_Slicing {};
		auto ResetDataFunc = [](const ArrayMultiDim::TArrayMultiDim<int, 10, 10>::CoordinateType& InCoord, int InLinearIdx, int& InOldData) -> int
		{
			return InLinearIdx + 1;
		};
		MultiDimTest_Slicing.SetData(ResetDataFunc);

		SliceTestType::SelfDynamicSizeType SliceRes = MultiDimTest_Slicing.Slice({{}, {2, 4}});
		const TArray<int> CoordExpectedData = {3, 4, 13, 14, 23, 24, 33, 34, 43, 44, 53, 54, 63, 64, 73, 74, 83, 84, 93, 94};
		int i = 0;
		for (auto It = SliceRes.CreateConstIterator(); It; ++It, ++i)
		{
			// UE_LOG(LogTemp, Display, TEXT("Function:[%hs] Data: %d"), __FUNCTION__, *It);
			TestEqual("(Storage order) Slice Result Data check: ", *It, CoordExpectedData[i]);
		}
		i = 0;
		SliceRes.LoopByCoord(
			[&i, &CoordExpectedData, this](const SliceTestType::CoordinateType& InCoordinate,
					SliceTestType::IndexType InLinearIdx,
					SliceTestType::IndexType InLoopCount,
					SliceTestType::StorageDataType& InData)
				{
					TestEqual("(Coord order) Slice Result Data check: ", InData, CoordExpectedData[i++]);  // Column first access, the data should be the same as above.
				}
			);

		// Change the storage order to 0, 1
		SliceRes.SetDimSize({10, 2}, {0, 1}, ArrayMultiDim::EResizeDataCopyPolicy::CoordinationCopy);
		i = 0;
		const TArray<int> StorageExpectedData = {3, 13, 23, 33, 43, 53, 63, 73, 83, 93, 4, 14, 24, 34, 44, 54, 64, 74, 84, 94};
		SliceRes.LoopByIndex(
			[&i, &StorageExpectedData, this](const SliceTestType::CoordinateType& InCoordinate,
					SliceTestType::IndexType InLinearIdx,
					SliceTestType::IndexType InLoopCount,
					SliceTestType::StorageDataType& InData)
				{
					TestEqual("(Storage order) (Change the storage order to 0, 1) Slice Result Data check: ", InData, StorageExpectedData[i++]);  // Same coordinate data should be the same.
				}
			);
		i = 0;
		SliceRes.LoopByCoord(
			[&i, &CoordExpectedData, this](const SliceTestType::CoordinateType& InCoordinate,
				SliceTestType::IndexType InLinearIdx,
				SliceTestType::IndexType InLoopCount,
				SliceTestType::StorageDataType& InData)
			{
				// UE_LOG(LogTemp, Display, TEXT("Function:[%hs] Coord loop coord: %s; InLinearIdx: %s; data: %s"),\
				// 	__FUNCTION__,\
				// 	*FString::JoinBy(InCoordinate, TEXT(", "),[](int32 Order) { return FString::FromInt(Order); }),\
				// 	*FString::FromInt(InLinearIdx),\
				// 	*FString::FromInt(InData));
				TestEqual("(Coord order) (Change the storage order to 0, 1) Slice Result Data check: ", InData, CoordExpectedData[i++]);  // Same coordinate data should be the same as previous storage order.
			}
		);
		PopContext();
	}

	// This block tests the "Mask data getting" function
	{
		PushContext("Slicing operator testing");
		// Prepare the test data to get.
		using SliceTestType = ArrayMultiDim::TArrayMultiDim<int, 10, 10>;
		SliceTestType MultiDimTest_Slicing {};
		auto ResetDataFunc = [](const ArrayMultiDim::TArrayMultiDim<int, 10, 10>::CoordinateType& InCoord, int InLinearIdx, int& InOldData) -> int
		{
			// UE_LOG(LogTemp, Display, TEXT("Function:[%hs] === MaskTest Data: %s ;; %d"), __FUNCTION__, *FString::JoinBy(InCoord, TEXT(", "), [](int32 Order) { return FString::FromInt(Order); }), InLinearIdx);
			return InLinearIdx + 1;  // Start from 1.
		};
		// MultiDimTest_Slicing.SetDimSize({10, 10}, {0, 1});
		// MultiDimTest_Slicing.SetDimSize({10, 10}, {1, 0});
		MultiDimTest_Slicing.SetData(ResetDataFunc);
		

		{
			// Prepare the mask data
			SliceTestType::MaskType MaskData {{
				{1, 1, 1, 1},
				{1, 1, 1, 1},
				{1, 1, 1, 1},
				{1, 1, 1, 1}
			}};

			// Get the data with the mask
			TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, {5, 5}, {1, 1});

			TArray<int> ExpectedGotData = { 45, 46, 47, 48, 55, 56, 57, 58, 65, 66, 67, 68, 75, 76, 77, 78 };
			for (int i = 0; i < MaskedData.Num(); ++i)
			{
				TestEqual("Full 1 Masked data getting: ", MaskedData[i], ExpectedGotData[i]);
			}
		}

		{
			// Prepare the mask data
			ArrayMultiDim::TArrayMultiDim<std::variant<bool, int>, -1, -1> MaskData {{
				{0, 1, 1, 0},
				{0, 1, 0, 1},
				{1, 0, 1, 1},
				{1, 0, 1, 0}
			}};

			// Get the data with the mask
			TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, {5, 5}, {1, 1});

			TArray<int> ExpectedGotData = { 46, 47, 56, 58, 65, 67, 68, 75, 77 };
			for (int i = 0; i < MaskedData.Num(); ++i)
			{
				TestEqual("Has 0 Masked data getting: ", MaskedData[i], ExpectedGotData[i]);
			}
		}
#if true
		// Use a big mask to test [out of range] behaviour
		{
			// Prepare the mask data with a very large mask. 28x16 size.
#pragma region BigMaskData
			ArrayMultiDim::TArrayMultiDim<std::variant<bool, int>, -1, -1> MaskData {{
				{1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0},
				{0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0},
				{0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1},
				{1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0},
				{0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1},
				{1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1},
				{0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1},
				{1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1},
				{1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1},
				{1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0},
				{0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1},
				{0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1},
				{0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0},
				{0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0},
				{0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1},
				{1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0},
				{0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1},
				{0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0},
				{0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0},
				{0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0},
				{0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1},
				{0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1},
				{1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0},
				{1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0},
				{0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1},
				{0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0},
				{0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0},
				{0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0},
				{1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0},
				{0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1},
				{1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
				{1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0},
				{1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1},

			}};
#pragma endregion BigMaskData

			{
				// Get the data with the mask
				TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, {5, 5}, {19, 15}, ArrayMultiDim::EBorderMode::NoPadding);
				TArray<int> ExpectedGotData = {1, 3, 4, 5, 6, 7, 9, 10, 11, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 26, 27, 28, 29, 31, 32, 35, 37, 38, 40, 41, 42, 49, 50, 51, 55, 56, 58, 59, 62, 63, 65, 69, 75, 78, 79, 80, 81, 84, 89, 94, 96, 97, 100};
				for (int i = 0; i < MaskedData.Num(); ++i)
				{
					TestEqual("NoPadding Masked data getting: ", MaskedData[i], ExpectedGotData[i]);
				}
			}
			{
				// Get the data with the mask
				TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, {5, 5}, {19, 15}, ArrayMultiDim::EBorderMode::RepeatBorder);
				TArray<int> ExpectedGotData = {61, 63, 67, 69, 70, 61, 64, 65, 62, 65, 66, 68, 70, 72, 77, 78, 79, 71, 73, 74, 77, 79, 80, 72, 73, 74, 75, 80, 85, 86, 81, 82, 84, 85, 86, 87, 90, 82, 83, 85, 87, 81, 91, 92, 93, 95, 96, 94, 95, 97, 98, 100, 93, 97, 98, 99, 100, 91, 2, 4, 6, 7, 8, 10, 1, 2, 3, 6, 10, 4, 5, 10, 12, 16, 18, 11, 12, 13, 14, 15, 16, 17, 19, 13, 14, 15, 16, 17, 18, 20, 11, 21, 23, 24, 27, 29, 30, 22, 23, 25, 30, 21, 24, 26, 27, 30, 21, 32, 36, 37, 39, 40, 31, 35, 36, 37, 38, 39, 34, 35, 36, 37, 38, 39, 40, 31, 41, 42, 43, 44, 45, 48, 49, 45, 47, 49, 50, 41, 44, 46, 47, 50, 41, 51, 52, 54, 57, 53, 54, 59, 52, 56, 57, 60, 51, 61, 62, 65, 67, 68, 70, 62, 63, 64, 66, 68, 69, 70, 61, 65, 66, 67, 68, 69, 70, 61, 72, 73, 74, 75, 76, 78, 80, 71, 75, 77, 72, 74, 79, 71, 81, 82, 86, 87, 89, 82, 83, 85, 88, 90, 81, 83, 86, 87, 88, 89, 90, 92, 94, 95, 97, 99, 92, 93, 94, 96, 97, 99, 94, 96, 98, 91, 2, 7, 9, 10, 1, 3, 4, 5, 6, 7, 9, 10, 1, 2, 3, 6, 9, 1, 15, 17, 18, 11, 13, 14, 15, 17, 18, 19, 20, 12, 13, 15, 19, 20, 22, 25, 26, 27, 28, 30, 21, 22, 23, 24, 26, 27, 28, 29, 21, 22, 23, 24, 25, 30, 35, 36, 37, 39, 31, 32, 35, 37, 38, 40, 34, 39, 31, 41, 42, 43, 46, 47, 48, 50, 41, 42, 49, 50, 41, 44, 45, 47, 48, 49, 50, 55, 57, 60, 51, 55, 56, 58, 59, 51, 52, 54, 55, 56, 57, 59, 51, 62, 63, 67, 68, 62, 63, 65, 69, 61, 65, 66, 67, 69, 74, 76, 77, 78, 80, 75, 78, 79, 80, 71, 72, 73, 74, 78, 80, 85, 81, 84, 89, 81, 82, 85, 87, 90, 93, 94, 95, 99, 94, 96, 97, 100, 94, 99, 100, 91, 3, 4, 5, 6, 10, 1, 2, 4, 6, 7, 1, 3, 5, 8, 1, 11, 13, 14, 18, 19, 20, 11, 12, 13, 15, 16, 17, 18, 19, 11, 12, 14, 15, 16, 17, 19, 21, 22, 24, 27, 30, 21, 22, 26, 27, 28, 29, 30, 21, 23, 24, 25, 30, 34, 36, 37, 39, 40, 31, 32, 33, 36, 37, 38, 34, 38, 40, 31, 46, 41, 42, 43, 45, 46, 47, 49, 42, 48, 49, 41, 52, 53, 54, 56, 58, 59, 51, 53, 54, 55, 58, 59, 60, 53, 54, 57, 58, 63, 66, 62, 63, 64, 65, 68, 70, 61, 63, 64, 66, 68, 69, 72, 73, 74, 76, 77, 78, 79, 80, 71, 77, 78, 79, 80, 73, 74, 78, 79, 80, 81, 86, 87, 88, 89, 87, 88, 89, 90, 81, 84, 85, 86, 87, 88, 89, 90, 94, 98, 99, 91, 92, 94, 95, 96, 100, 91, 94, 95, 98, 91, 1, 8, 9, 10, 1, 2, 3, 4, 5, 7, 8, 9, 10, 2, 6, 1, 11, 12, 15, 16, 17, 19, 14, 16, 17, 12, 13, 15, 18, 20, 21, 22, 26, 30, 21, 23, 24, 25, 27, 29, 30, 21, 23, 25, 26, 30, 21};
				for (int i = 0; i < MaskedData.Num(); ++i)
				{
					TestEqual("RepeatBorder Masked data getting: ", MaskedData[i], ExpectedGotData[i]);
				}
			}
			{
				// Get the data with the mask
				TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, {5, 5}, {19, 15}, ArrayMultiDim::EBorderMode::ReflectBorder);
				TArray<int> ExpectedGotData = {70, 68, 64, 62, 61, 61, 64, 65, 69, 66, 65, 63, 61, 79, 74, 73, 72, 71, 73, 74, 77, 79, 80, 79, 78, 77, 76, 71, 86, 85, 81, 82, 84, 85, 86, 87, 90, 89, 88, 86, 84, 81, 100, 99, 98, 96, 95, 94, 95, 97, 98, 100, 98, 94, 93, 92, 91, 91, 99, 97, 95, 94, 93, 91, 91, 92, 93, 96, 100, 97, 96, 91, 89, 85, 83, 81, 82, 83, 84, 85, 86, 87, 89, 88, 87, 86, 85, 84, 83, 81, 81, 80, 78, 77, 74, 72, 71, 72, 73, 75, 80, 80, 77, 75, 74, 71, 71, 69, 65, 64, 62, 61, 61, 65, 66, 67, 68, 69, 67, 66, 65, 64, 63, 62, 61, 61, 60, 59, 58, 57, 56, 53, 52, 55, 57, 59, 60, 60, 57, 55, 54, 51, 51, 50, 49, 47, 44, 43, 44, 49, 49, 45, 44, 41, 41, 40, 39, 36, 34, 33, 31, 32, 33, 34, 36, 38, 39, 40, 40, 36, 35, 34, 33, 32, 31, 31, 29, 28, 27, 26, 25, 23, 21, 21, 25, 27, 29, 27, 22, 21, 20, 19, 15, 14, 12, 12, 13, 15, 18, 20, 20, 18, 15, 14, 13, 12, 11, 9, 7, 6, 4, 2, 2, 3, 4, 6, 7, 9, 7, 5, 3, 1, 9, 4, 2, 1, 1, 3, 4, 5, 6, 7, 9, 10, 10, 9, 8, 5, 2, 1, 16, 14, 13, 11, 13, 14, 15, 17, 18, 19, 20, 19, 18, 16, 12, 11, 29, 26, 25, 24, 23, 21, 21, 22, 23, 24, 26, 27, 28, 29, 30, 29, 28, 27, 26, 21, 36, 35, 34, 32, 31, 32, 35, 37, 38, 40, 37, 32, 31, 50, 49, 48, 45, 44, 43, 41, 41, 42, 49, 50, 50, 47, 46, 44, 43, 42, 41, 56, 54, 51, 51, 55, 56, 58, 59, 60, 59, 57, 56, 55, 54, 52, 51, 69, 68, 64, 63, 62, 63, 65, 69, 70, 66, 65, 64, 62, 77, 75, 74, 73, 71, 75, 78, 79, 80, 80, 79, 78, 77, 73, 71, 86, 81, 84, 89, 90, 89, 86, 84, 81, 98, 97, 96, 92, 94, 96, 97, 100, 97, 92, 91, 91, 98, 97, 96, 95, 91, 91, 92, 94, 96, 97, 100, 98, 96, 93, 91, 90, 88, 87, 83, 82, 81, 81, 82, 83, 85, 86, 87, 88, 89, 90, 89, 87, 86, 85, 84, 82, 80, 79, 77, 74, 71, 71, 72, 76, 77, 78, 79, 80, 80, 78, 77, 76, 71, 67, 65, 64, 62, 61, 61, 62, 63, 66, 67, 68, 67, 63, 61, 61, 55, 51, 52, 53, 55, 56, 57, 59, 59, 53, 52, 51, 49, 48, 47, 45, 43, 42, 41, 43, 44, 45, 48, 49, 50, 48, 47, 44, 43, 38, 35, 32, 33, 34, 35, 38, 40, 40, 38, 37, 35, 33, 32, 29, 28, 27, 25, 24, 23, 22, 21, 21, 27, 28, 29, 30, 28, 27, 23, 22, 21, 20, 15, 14, 13, 12, 17, 18, 19, 20, 20, 17, 16, 15, 14, 13, 12, 11, 7, 3, 2, 1, 2, 4, 5, 6, 10, 10, 7, 6, 3, 1, 10, 3, 2, 1, 1, 2, 3, 4, 5, 7, 8, 9, 10, 9, 5, 1, 20, 19, 16, 15, 14, 12, 14, 16, 17, 19, 18, 16, 13, 11, 30, 29, 25, 21, 21, 23, 24, 25, 27, 29, 30, 30, 28, 26, 25, 21, 21};
				for (int i = 0; i < MaskedData.Num(); ++i)
				{
					TestEqual("ReflectBorder Masked data getting: ", MaskedData[i], ExpectedGotData[i]);
				}
			}
			{
				// Get the data with the mask
				TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, {5, 5}, {19, 15}, ArrayMultiDim::EBorderMode::Reflect101Border);
				TArray<int> ExpectedGotData = {49, 49, 45, 43, 42, 41, 44, 45, 48, 45, 44, 42, 42, 60, 55, 54, 53, 51, 53, 54, 57, 59, 60, 58, 57, 56, 55, 52, 67, 66, 61, 62, 64, 65, 66, 67, 70, 68, 67, 65, 63, 63, 79, 80, 79, 77, 76, 74, 75, 77, 78, 80, 77, 73, 72, 71, 72, 73, 90, 88, 86, 85, 84, 82, 81, 82, 83, 86, 90, 86, 85, 82, 100, 96, 94, 91, 92, 93, 94, 95, 96, 97, 99, 97, 96, 95, 94, 93, 92, 92, 93, 89, 89, 88, 85, 83, 82, 82, 83, 85, 90, 89, 86, 84, 83, 82, 83, 80, 76, 75, 73, 72, 71, 75, 76, 77, 78, 79, 76, 75, 74, 73, 72, 71, 72, 73, 69, 70, 69, 68, 67, 64, 63, 65, 67, 69, 70, 69, 66, 64, 63, 62, 63, 59, 60, 58, 55, 53, 54, 59, 58, 54, 53, 52, 53, 49, 50, 47, 45, 44, 42, 42, 43, 44, 46, 48, 49, 50, 49, 45, 44, 43, 42, 41, 42, 43, 40, 39, 38, 37, 36, 34, 32, 31, 35, 37, 38, 36, 31, 33, 29, 30, 26, 25, 23, 22, 23, 25, 28, 30, 29, 27, 24, 23, 22, 21, 22, 20, 18, 17, 15, 13, 12, 13, 14, 16, 17, 19, 16, 14, 12, 13, 10, 5, 3, 2, 1, 3, 4, 5, 6, 7, 9, 10, 9, 8, 7, 4, 1, 3, 17, 15, 14, 11, 13, 14, 15, 17, 18, 19, 20, 18, 17, 15, 11, 12, 30, 27, 26, 25, 24, 22, 21, 22, 23, 24, 26, 27, 28, 29, 29, 28, 27, 26, 25, 22, 37, 36, 35, 33, 31, 32, 35, 37, 38, 40, 36, 31, 33, 49, 50, 49, 46, 45, 44, 42, 41, 42, 49, 50, 49, 46, 45, 43, 42, 41, 42, 57, 55, 52, 51, 55, 56, 58, 59, 59, 58, 56, 55, 54, 53, 51, 53, 70, 69, 65, 64, 62, 63, 65, 69, 69, 65, 64, 63, 61, 78, 76, 75, 74, 72, 75, 78, 79, 80, 79, 78, 77, 76, 72, 72, 87, 81, 84, 89, 89, 88, 85, 83, 82, 99, 98, 97, 93, 94, 96, 97, 100, 96, 91, 92, 93, 89, 88, 87, 86, 82, 81, 82, 84, 86, 87, 89, 87, 85, 82, 83, 79, 79, 78, 74, 73, 72, 71, 72, 73, 75, 76, 77, 78, 79, 79, 78, 76, 75, 74, 73, 71, 69, 70, 68, 65, 62, 61, 62, 66, 67, 68, 69, 70, 69, 67, 66, 65, 62, 58, 56, 55, 53, 52, 51, 52, 53, 56, 57, 58, 56, 52, 52, 53, 46, 41, 42, 43, 45, 46, 47, 49, 48, 42, 41, 43, 40, 39, 38, 36, 34, 33, 31, 33, 34, 35, 38, 39, 40, 37, 36, 33, 32, 29, 26, 22, 23, 24, 25, 28, 30, 29, 27, 26, 24, 22, 21, 20, 19, 18, 16, 15, 14, 13, 12, 11, 17, 18, 19, 20, 17, 16, 12, 11, 12, 9, 6, 5, 4, 3, 7, 8, 9, 10, 9, 6, 5, 4, 3, 2, 1, 2, 18, 14, 13, 11, 12, 14, 15, 16, 20, 19, 16, 15, 12, 13, 29, 24, 23, 22, 21, 22, 23, 24, 25, 27, 28, 29, 30, 28, 24, 23, 39, 40, 37, 36, 35, 33, 34, 36, 37, 38, 37, 35, 32, 32, 49, 50, 46, 42, 41, 43, 44, 45, 47, 49, 50, 49, 47, 45, 44, 42, 43};
				for (int i = 0; i < MaskedData.Num(); ++i)
				{
					TestEqual("Reflect101Border Masked data getting: ", MaskedData[i], ExpectedGotData[i]);
				}
			}

		}
		
		// Check apply point out-of-range. 
		{
			// Prepare the mask data
			ArrayMultiDim::TArrayMultiDim<std::variant<bool, int>, -1, -1> MaskData {{
				{1, 1, 1, 1},
				{1, 1, 1, 1},
				{1, 1, 1, 1},
				{1, 1, 1, 1}
			}};

			{
				TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, {500, -500}, {-100, 100});
				TestEqual("Apply point out-of-range (NoPadding MODE): ", MaskedData.Num(), 0);
			}
			{
				TArray<int> MaskedData = MultiDimTest_Slicing.GetElementsByMask(MaskData, {500, -500}, {-100, 100}, ArrayMultiDim::RepeatBorder);
				TestEqual("Apply point out-of-range (RepeatBorder MODE): ", MaskedData.Num(), 16);
			}
		}
		
		PopContext();
#endif
	}
	return true;
}
