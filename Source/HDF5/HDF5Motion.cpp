#include "HDF5Motion.h"
#include "HDF5Module.h"
#include "Misc/FileHelper.h"
#include "hdf5.h"

// Ref: Compression datasets https://support.hdfgroup.org/documentation/hdf5/latest/_l_b_com_dset.html
// Ref: Chunking https://support.hdfgroup.org/documentation/hdf5/latest/hdf5_chunking.html
// Ref: Source code examples: https://support.hdfgroup.org/documentation/hdf5/latest/_l_b_examples.html

bool WriteTMapToCompressedH5(const TMap<FString, TArray<TArray<float>>>& InputData, int CompressionLevel, const FString& OutHDF5File)
{
    if (InputData.IsEmpty())
    {
        UE_LOG(LogHDF5, Error, TEXT("Failed to create HDF5 file: %s because InputData is empty"), *OutHDF5File);
        return false;
    };

    if (CompressionLevel < 1 || CompressionLevel > 9)
    {
        UE_LOG(LogHDF5, Error, TEXT("Failed to create HDF5 file: %s because CompressionLevel %d is invalid must be in [1, 9]"), *OutHDF5File, CompressionLevel);
        return false;
    };

    H5Eclear(H5E_DEFAULT);

    // 1. 创建HDF5文件（H5F_ACC_TRUNC：覆盖已有文件）
    hid_t file_id = H5Fcreate(TCHAR_TO_UTF8(*OutHDF5File), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id == -1)
    {
        UE_LOG(LogHDF5, Error, TEXT("Failed to create HDF5 file: %s"), *OutHDF5File);
        return false;
    }

    herr_t status = -1;

    // 2. 遍历TMap，创建组和数据集
    for (const auto& Pair : InputData)
    {
        const FString& GroupName = Pair.Key;                    // HDF5组名（TMap的键）
        const TArray<TArray<float>>& TwoDArray = Pair.Value;    // 二维数组

        // 跳过空数组
        if (TwoDArray.Num() == 0 || TwoDArray[0].Num() == 0)
        {
            UE_LOG(LogHDF5, Warning, TEXT("Empty array for key: %s"), *GroupName);
            continue;
        };

        // 3. 创建HDF5组（以TMap的键为组名）
        hid_t group_id = H5Gcreate2(file_id, TCHAR_TO_UTF8(*GroupName), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (group_id == -1)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to create group: %s"), *GroupName);
            return false;
        };


        // 4. 准备数据集维度（二维数组：[行数][列数]）
        hsize_t dims[2] = { (hsize_t)TwoDArray.Num(), (hsize_t)TwoDArray[0].Num() };  // 假设子数组长度一致（非一致需特殊处理）
        hid_t dataspace_id = H5Screate_simple(2, dims, nullptr);  // 2维空间
        if (dataspace_id == -1) 
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to create simple dataspace for group %s"), *GroupName);
            return false;
        }


        // 5. Create dataset creation property list
        hid_t dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
        if (dcpl_id == -1)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to create simple dataspace for group %s"), *GroupName);
            return false;
        }

        // 6. Set chunk for dcpl. Chunk must be set for any compression. Maybe we don't need to do compression. Just add the code here.
        int ChunkDim = (hsize_t)FMath::Min(TwoDArray[0].Num(), 3);

        hsize_t chunk_dims[2] = { ChunkDim, ChunkDim };
        if(H5Pset_chunk(dcpl_id, 2, chunk_dims) < 0)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to set chunk for group %s"), *GroupName);
            return false;
        }

        // 7. Set deflate to dcpl
        if(H5Pset_deflate(dcpl_id, CompressionLevel) < 0)  // 设置deflate/gzip压缩
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to set deflate/gzip compression level for group %s"), *GroupName);
            return false;
        }

        // 8. 创建数据集（float类型，带压缩属性）
        hid_t dataset_id = H5Dcreate2(
            group_id,                    // 所属组ID
            "data",  // 数据集名（固定为"data"）
            H5T_NATIVE_FLOAT,           // 数据类型（UE float对应H5T_NATIVE_FLOAT）
            dataspace_id,               // 数据空间
            H5P_DEFAULT,
            dcpl_id,                    // 压缩属性列表
            H5P_DEFAULT
        );

        if (dataset_id == -1)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to create dataset for group %s"), *GroupName);
            return false;
        }

        // 7.  flatten二维数组为连续内存（HDF5要求连续存储）
        TArray<float> FlattenedData;
        FlattenedData.Reserve(dims[0] * dims[1]);
        for (const auto& Row : TwoDArray)
        {
            FlattenedData.Append(Row);  // 假设所有Row长度相同（不同长度需额外存储行列数）
        }

        // 8. 写入数据到HDF5
        if (H5Dwrite(
            dataset_id,
            H5T_NATIVE_FLOAT,
            H5S_ALL,
            H5S_ALL,
            H5P_DEFAULT,
            FlattenedData.GetData()  // 连续内存首地址
        ) < 0)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to write data into dataset for group %s"), *GroupName);
            return false;
        }

        // 9. 释放当前组相关资源
        H5Dclose(dataset_id);
        H5Pclose(dcpl_id);
        H5Sclose(dataspace_id);
        H5Gclose(group_id);
    }

    // 10. 关闭文件
    H5Fclose(file_id);
    
    UE_LOG(LogHDF5, Log, TEXT("Successfully wrote TMap to compressed HDF5: %s"), *OutHDF5File);
    
    return true;
}

bool ReadCompressedH5ToTMap(const FString& InHDF5File, TMap<FString, TArray<TArray<float>>>& OutData)
{
    OutData.Empty();  // 清空输出Map


    // 1. 打开HDF5文件（只读模式）
    H5Eclear(H5E_DEFAULT);
    hid_t file_id = H5Fopen(TCHAR_TO_UTF8(*InHDF5File), H5F_ACC_RDONLY, H5P_DEFAULT);

    if (file_id < 0)
    {
        UE_LOG(LogHDF5, Error, TEXT("Failed to open HDF5 file: %s"), *InHDF5File);
        return false;
    }

    // 2. 遍历根目录下所有组（对应TMap的键）
    hid_t root_group = H5Gopen2(file_id, "/", H5P_DEFAULT);
    if (root_group < 0)
    {
        UE_LOG(LogHDF5, Error, TEXT("Failed to get root group from %s"), *InHDF5File);
        H5Fclose(file_id);
        return false;
    }

    H5G_info_t group_info;
    if(H5Gget_info(root_group, &group_info) < 0)  // 获取根目录信息
    {
        UE_LOG(LogHDF5, Error, TEXT("Failed to get group info from root_group: %s"), *InHDF5File);
        H5Gclose(root_group);
        H5Fclose(file_id);
        return false;
    }

    for (hsize_t i = 0; i < group_info.nlinks; ++i)
    {
        char group_name[256];
        H5Lget_name_by_idx(file_id, ".", H5_INDEX_NAME, H5_ITER_INC, i, group_name, sizeof(group_name), H5P_DEFAULT);
        FString GroupName = FString(UTF8_TO_TCHAR(group_name));

        // 3. 打开组
        hid_t group_id = H5Gopen2(file_id, group_name, H5P_DEFAULT);
        if (group_id < 0)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to open group %s for %s"), *GroupName, *InHDF5File);
            H5Gclose(root_group);
            H5Fclose(file_id);
            return false;
        };

        // 4. 打开组内的"data"数据集
        hid_t dataset_id = H5Dopen2(group_id, "data", H5P_DEFAULT);
        if (dataset_id < 0)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to get open \"data\" dataset for group %s"), *GroupName);
            H5Gclose(group_id);
            H5Gclose(root_group);
            H5Fclose(file_id);
            return false;
        };

        // 5. 获取数据集维度（行数、列数）
        hid_t dataspace_id = H5Dget_space(dataset_id);
        if (dataspace_id < 0)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to get open \"data\" dataset for group %s"), *GroupName);
            H5Dclose(dataset_id);
            H5Gclose(group_id);
            H5Gclose(root_group);
            H5Fclose(file_id);
            return false;
        };

        hsize_t dims[2];
        int rank = H5Sget_simple_extent_dims(dataspace_id, dims, nullptr);  // rank=2（二维）
        if (rank < 0)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to get dataspace dims for group %s"), *GroupName);
            H5Dclose(dataspace_id);
            H5Sclose(dataset_id);
            H5Gclose(group_id);
            H5Gclose(root_group);
            H5Fclose(file_id);
            return false;
        };

        int32 NumRows = (int32)dims[0];
        int32 NumCols = (int32)dims[1];

        // 6. 读取数据到连续内存
        TArray<float> FlattenedData;
        FlattenedData.SetNumUninitialized(NumRows * NumCols);
        if (H5Dread(dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, FlattenedData.GetData()) < 0)
        {
            UE_LOG(LogHDF5, Error, TEXT("Failed to read dataset to TArray<float> %s"), *GroupName);
            H5Dclose(dataspace_id);
            H5Sclose(dataset_id);
            H5Gclose(group_id);
            H5Gclose(root_group);
            H5Fclose(file_id);
            return false;
        }

        // 7. 恢复为二维数组（TArray<TArray<float>>）
        TArray<TArray<float>> TwoDArray;
        TwoDArray.Reserve(NumRows);
        for (int32 Row = 0; Row < NumRows; ++Row)
        {
            TArray<float> CurrentRow;
            CurrentRow.Append(&FlattenedData[Row * NumCols], NumCols);
            TwoDArray.Add(CurrentRow);
        }

        // 8. 添加到TMap
        OutData.Add(GroupName, TwoDArray);

        // 9. 释放资源
        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
        H5Gclose(group_id);
    }

    // 10. 关闭文件
    H5Gclose(root_group);
    H5Fclose(file_id);
    //UE_LOG(LogHDF5, Log, TEXT("Successfully read TMap from HDF5 %s: %d groups loaded."), *InHDF5File, OutData.Num());
    return true;
}

bool FHDF5Motion::ReadFile(const FString& InHDF5File)
{
    bool Res = ReadCompressedH5ToTMap(InHDF5File, MotionObject);
    FrameCount = 0;
    for (auto& Item : MotionObject)
    {
        FrameCount = FMath::Max(FrameCount, Item.Value.Num());
    }
    return Res;
}

bool FHDF5Motion::WriteFile(const FString& OutFile)
{
    bool Res = WriteTMapToCompressedH5(MotionObject, 6, OutFile);
    return Res;
}

float FHDF5Motion::GetTotalLength() const
{
	return FrameCount * (1.0 / GetFPS());
}

int32 FHDF5Motion::GetTotalFrames() const
{
	return FrameCount;
}

float FHDF5Motion::GetFPS() const
{
	return 30.0;
}

float FHDF5Motion::GetTime(int32 FrameIndex) const
{
	return FrameIndex * (1.0 / GetFPS());
}

int32 FHDF5Motion::GetFrameIndex(float Time) const
{
	return Time * GetFPS();
}
