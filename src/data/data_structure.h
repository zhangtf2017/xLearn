//------------------------------------------------------------------------------
// Copyright (c) 2016 by contributors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------

/*
Author: Chao Ma (mctt90@gmail.com)

This file defines the basic data structures used by xLearn.
*/

#ifndef XLEARN_DATA_DATA_STRUCTURE_H_
#define XLEARN_DATA_DATA_STRUCTURE_H_

#include <vector>
#include <string>

#include "src/base/common.h"
#include "src/base/stl-util.h"
#include "src/base/file_util.h"

namespace xLearn {

//------------------------------------------------------------------------------
// We use 32 bits float to store real number, such as the model
// parameter and the gradient.
//------------------------------------------------------------------------------
typedef float real_t;

//------------------------------------------------------------------------------
// We use 32 bits unsigned int to store feature index.
//------------------------------------------------------------------------------
typedef uint32 index_t;

//------------------------------------------------------------------------------
// Node is used to store information for each feature.
// For tasks like lr and fm, we just need to store the feature id
// and feature value, while we also need to store the field id for
// the ffm task.
//------------------------------------------------------------------------------
struct Node {
  index_t field_id;  /* Start from 0 */
  index_t feat_id;   /* Start from 0, which is the bias term */
  real_t feat_val;   /* Can be numeric or catagorical feature */
};

//------------------------------------------------------------------------------
// SparseRow is used to store one line of the training data, which
// is a vector of the Node data structure.
//------------------------------------------------------------------------------
typedef std::vector<Node> SparseRow;

//------------------------------------------------------------------------------
// DMatrix (data matrix) is used to store a batch of training dataset.
// It can be the whole data set used in in-memory training, and or working
// set in on-disk training, because for many large-scale ML problems, we
// cannot load all the training data into memory at once. So we can load a
// small batch of dataset into the DMatrix at each iteration.
// We can use the DMatrix like this:
//
//    DMatrix matrix;
//    matrix.ResetMatrix(10);   /* Init 10 rows */
//    for (int i = 0; i < 10; ++i) {
//      matrix.Y[i] = ...  /* set y */
//      matrix.row[i] = new SparseRow;
//      matrix.AddNode(i, feat_id, feat_val, field_id);
//    }
//    matrix.Serialize("/tmp/test.bin");    /* Serialize matrix to file */
//    matrix.Release();
//    matrix.Deserialize("/tmp/test.bin");  /* Deserialize matrix from file */
//
//    /* We can access the matrix like this */
//    for (int i = 0; i < matrix.row_length; ++i) {
//      ... matrix.Y[i] ..   /* access y */
//      SparseRow *row = matrix.row[i];
//      for (SparseRow::iterator iter = row->begin();
//           iter != row->end(); ++iter) {
//        ... iter->field_id ...   /* access field_id */
//        ... iter->feat_id ...    /* access feat_id */
//        ... iter->feat_val ...   /* access feat_val */
//      }
//    }
//------------------------------------------------------------------------------
struct DMatrix {
  // Constructor and Destructor
  DMatrix() { }
  ~DMatrix() { Release(); }

  // Reset memory for the DMatrix
  // This function will first release the original
  // memory of the DMatrix, and then re-allocate memory
  // for that. In this function, Y will be initialized
  // to 0 and row will be initialized to a NULL pointer
  void ResetMatrix(index_t length) {
    CHECK_GE(length, 0);
    this->Release();
    row_length = length;
    row.resize(length, nullptr);
    Y.resize(length, 0);
  }

  // Release memory for DMatrix
  // Note that a typical alternative that forces a
  // reallocation is to use swap(), instead of using clear()
  void Release() {
    row_length = 0;
    std::vector<real_t>().swap(Y);
    STLDeleteElementsAndClear(&row);
    std::vector<SparseRow*>().swap(row);
  }

  // Add node to matrix
  void AddNode(index_t row_id,  index_t feat_id,
               real_t feat_val, index_t field_id = 0) {
    CHECK_GT(row_length, row_id);
    CHECK_NOTNULL(row[row_id]);
    Node node;
    node.field_id = field_id;
    node.feat_id = feat_id;
    node.feat_val = feat_val;
    row[row_id]->push_back(node);
  }

  // Serialize current DMatrix to disk file
  void Serialize(const std::string& filename) {
    CHECK(!filename.empty());
    CHECK_EQ(row_length, row.size());
    CHECK_EQ(row_length, Y.size());
    FILE* file = OpenFileOrDie(filename.c_str(), "w");
    // Write row_length
    WriteDataToDisk(file,
         (char*)&row_length,
         sizeof(row_length));
    // Write row
    for (int i = 0; i < row_length; ++i) {
      WriteVectorToFile(file, *(row[i]));
    }
    // Write Y
    WriteVectorToFile(file, Y);
    Close(file);
  }

  // Deserialize the DMatrix from disk file
  void Deserialize(const std::string& filename) {
    CHECK(!filename.empty());
    this->Release();
    FILE* file = OpenFileOrDie(filename.c_str(), "r");
    // Read row_length
    ReadDataFromDisk(file,
         (char*)&row_length,
         sizeof(row_length));
    // Read row
    row.resize(row_length);
    for (int i = 0; i < row_length; ++i) {
      row[i] = new SparseRow;
      ReadVectorFromFile(file, *(row[i]));
    }
    // Read Y
    ReadVectorFromFile(file, Y);
    Close(file);
  }

  index_t row_length;           /* Row length of current matrix */
  std::vector<SparseRow*> row;  /* Using pointer to implement zero-copy */
  std::vector<real_t> Y;        /* 0 or -1 for negative and +1 for positive
                                   example, and others for regression */
};

}  // namespace xLearn

#endif  // XLEARN_DATA_DATA_STRUCTURE_H_
