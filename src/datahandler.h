#ifndef DATAHANDLER_H_
#define DATAHANDLER_H_
#include "layer.h"
#include <random>

class DataHandler {
 public:
  DataHandler(const config::DatasetConfig& config);
  ~DataHandler() {}

  virtual void GetBatch(vector<Layer*>& data_layers) = 0;

  static DataHandler* ChooseDataHandler(const string& config_file);

  int GetBatchSize() const { return batch_size_; }
  int GetDataSetSize() const { return dataset_size_; }
  virtual void Seek(int row) { row_ = row; }

  // Number of positions in same image.
  int GetNumPositions() const { return num_positions_;}

 protected:
  void SetupShuffler(int dataset_size);

  const int gpu_id_;
  int batch_size_, dataset_size_, row_;
  const string base_dir_, mean_file_;
  Matrix rand_perm_indices_;
  const bool randomize_cpu_, randomize_gpu_;
  int num_positions_; 
};

class DummyDataHandler : public DataHandler {
 public:
  DummyDataHandler(const config::DatasetConfig& config) : DataHandler(config) {}
  void GetBatch(vector<Layer*>& data_layers);
};

// TODO: Make a real STL-like iterator.
// Iterates over a dataset in an hdf5 file sequentially.
class HDF5Iterator {
 public:
  HDF5Iterator(const string& file_name, const string& dataset_name);
  ~HDF5Iterator();

  // Reads a row from disk into data_ptr.
  // Sufficient memory should already be allocated.
  void GetNext(void* data_ptr, const int row);

  // Reads a row using the internal state row_.
  virtual void GetNext(void* data_ptr);

  void Seek(int row) { row_ = row; }
  int GetDatasetSize() const { return dataset_size_;}
  int GetDims() const { return num_dims_;}

 private:

  int num_dims_, dataset_size_, row_;
  hid_t file_, dataset_, dapl_id_, m_dataspace_, type_;
  hsize_t start_[2], count_[2];
};

// Accesses rows randomly in a dataset in an hdf5 file.
class HDF5RandomAccessor : public HDF5Iterator {
 public:

  // chunk_size : Number of rows to read sequentially after a random Seek.
  // To get a random permutation this must be set to 1. However, this will lead
  // to a lot of random disk access which will slow things down. So it might be
  // better to set this to a higher value like 64 or 128.
  HDF5RandomAccessor(const string& file_name, const string& dataset_name, int chunk_size);
  ~HDF5RandomAccessor();
  virtual void GetNext(void* data_ptr);

 private:
  default_random_engine generator_;
  uniform_int_distribution<int> * distribution_;
  const int chunk_size_;
  int ind_;
};


// Iterates over multiple datasets in an hdf5 file.
// All datasets must have the same number of rows.
class HDF5MultiIterator {
 public:
  HDF5MultiIterator(const string& file_name, const vector<string>& dataset_names);
  ~HDF5MultiIterator();
  
  // Reads a row from disk into data_ptr.
  // Sufficient memory should already be allocated.
  void GetNext(vector<void*>& data_ptr, const int row);

  // Reads a row using the internal state row_.
  virtual void GetNext(vector<void*>& data_ptr);

  void Seek(int row) { row_ = row; }
  int GetDatasetSize() const { return dataset_size_;}
  int GetDims(int i) const { return it_[i]->GetDims();}
  int GetNumIterators() const { return num_it_;}

 private:

  vector<HDF5Iterator*> it_;
  const int num_it_;
  int dataset_size_, row_;
};

class HDF5RandomMultiAccessor : public HDF5MultiIterator {
 public:
  HDF5RandomMultiAccessor(const string& file_name,
                          const vector<string>& dataset_names, int chunk_size);
  ~HDF5RandomMultiAccessor();
  virtual void GetNext(vector<void*>& data_ptr);

 private:
  default_random_engine generator_;
  uniform_int_distribution<int> * distribution_;
  const int chunk_size_;
  int ind_;
};


class DataWriter {
 public:
  DataWriter(const string& output_file, const int dataset_size);
  ~DataWriter();
  virtual void AddStream(const string& name, const int numdims);
  virtual void Write(Matrix& mat, const int data_id, const int rows);

 private:
  const string output_file_;
  const int dataset_size_;
  vector<int> numdims_;
  vector<hid_t> dataset_handle_, dataspace_handle_;
  vector<int> current_row_;
  hid_t file_;
  int num_streams_;
};

class AveragedDataWriter : public DataWriter {
 public:
  AveragedDataWriter(const string& output_file, const int dataset_size,
                     const int avg_after, int max_batchsize);
  ~AveragedDataWriter();
  virtual void AddStream(const string& name, const int numdims);
  virtual void Write(Matrix& mat, const int data_id, const int rows);
 private:
  const int avg_after_, max_batchsize_;
  vector<Matrix*> buf_;
  vector<int> counter_;
};

class SequentialAveragedDataWriter : public DataWriter {
 public:
  SequentialAveragedDataWriter(const string& output_file, const int dataset_size,
                               const int avg_after);
  ~SequentialAveragedDataWriter();
  virtual void AddStream(const string& name, const int numdims);
  virtual void Write(Matrix& mat, const int data_id, const int rows);

 private:
  const int avg_after_, dataset_size_;
  vector<Matrix*> buf_;
  int consumed_, num_rows_written_;
};

#endif

