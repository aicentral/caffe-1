#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#endif  // USE_OPENCV
#include <stdint.h>

#include <algorithm>
#include <map>
#include <vector>

#include "caffe/data_transformer.hpp"
#include "caffe/layers/annotated_word_data_layer.hpp"
#include "caffe/util/benchmark.hpp"
#include "caffe/util/sampler.hpp"

namespace caffe {

template <typename Dtype>
AnnotatedWordDataLayer<Dtype>::AnnotatedWordDataLayer(const LayerParameter& param)
  : BasePrefetchingDataLayer<Dtype>(param),
    reader_(param) {
}

template <typename Dtype>
AnnotatedWordDataLayer<Dtype>::~AnnotatedWordDataLayer() {
  this->StopInternalThread();
}

template <typename Dtype>
void AnnotatedWordDataLayer<Dtype>::DataLayerSetUp(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  const int batch_size = this->layer_param_.data_param().batch_size();
  const AnnotatedWordDataParameter& anno_data_param =
      this->layer_param_.annotated_word_data_param();
  for (int i = 0; i < anno_data_param.batch_sampler_size(); ++i) {
    batch_samplers_.push_back(anno_data_param.batch_sampler(i));
  }
  label_map_file_ = anno_data_param.label_map_file();

  // Read a data point, and use it to initialize the top blob.
  AnnotatedDatum& anno_datum = *(reader_.full().peek());

  // Use data_transformer to infer the expected blob shape from anno_datum.
  vector<int> top_shape =
      this->data_transformer_->InferBlobShape(anno_datum.datum());
  this->transformed_data_.Reshape(top_shape);
  // Reshape top[0] and prefetch_data according to the batch_size.
  top_shape[0] = batch_size;
  top[0]->Reshape(top_shape);
  for (int i = 0; i < this->PREFETCH_COUNT; ++i) {
    this->prefetch_[i].data_.Reshape(top_shape);
  }
  LOG(INFO) << "output data size: " << top[0]->num() << ","
      << top[0]->channels() << "," << top[0]->height() << ","
      << top[0]->width();
  // label
  if (this->output_labels_) {
    has_anno_type_ = anno_datum.has_type();
    vector<int> label_shape(4, 1);
    if (has_anno_type_) {
      anno_type_ = anno_datum.type();
      // Infer the label shape from anno_datum.AnnotationGroup().
      int num_bboxes = 0;
      if (anno_type_ == AnnotatedDatum_AnnotationType_BBOX) {
        // Since the number of bboxes can be different for each image,
        // we store the bbox information in a specific format. In specific:
        // All bboxes are stored in one spatial plane (num and channels are 1)
        // And each row contains one and only one box in the following format:
        // [item_id, group_label, instance_id, xmin, ymin, xmax, ymax, diff]
        // Note: Refer to caffe.proto for details about group_label and
        // instance_id.
        for (int g = 0; g < anno_datum.annotation_group_size(); ++g) {
          num_bboxes += anno_datum.annotation_group(g).annotation_size();
        }
        label_shape[0] = 1;
        label_shape[1] = 1;
        // BasePrefetchingDataLayer<Dtype>::LayerSetUp() requires to call
        // cpu_data and gpu_data for consistent prefetch thread. Thus we make
        // sure there is at least one bbox.
        label_shape[2] = std::max(num_bboxes, 1);
        label_shape[3] = 8+25;
      } else {
        LOG(FATAL) << "Unknown annotation type.";
      }
    } else {
      label_shape[0] = batch_size;
    }
    top[1]->Reshape(label_shape);
    for (int i = 0; i < this->PREFETCH_COUNT; ++i) {
      this->prefetch_[i].label_.Reshape(label_shape);
    }
  }
}

// This function is called on prefetch thread
template<typename Dtype>
void AnnotatedWordDataLayer<Dtype>::load_batch(Batch<Dtype>* batch) {
  CPUTimer batch_timer;
  batch_timer.Start();
  double read_time = 0;
  double trans_time = 0;
  CPUTimer timer;
  CHECK(batch->data_.count());
  CHECK(this->transformed_data_.count());

    std::map<char,int> label_to_char_map;

  label_to_char_map['0'] = 0;
  label_to_char_map['1'] = 1;
  label_to_char_map['2'] = 2;
  label_to_char_map['3'] = 3;
  label_to_char_map['4'] = 4;
  label_to_char_map['5'] = 5;
  label_to_char_map['6'] = 6;
  label_to_char_map['7'] = 7;
  label_to_char_map['8'] = 8;
  label_to_char_map['9'] = 9;
  label_to_char_map['a'] = 10;
  label_to_char_map['b'] = 11;
  label_to_char_map['c'] = 12;
  label_to_char_map['d'] = 13;
  label_to_char_map['e'] = 14;
  label_to_char_map['f'] = 15;
  label_to_char_map['g'] = 16;
  label_to_char_map['h'] = 17;
  label_to_char_map['i'] = 18;
  label_to_char_map['j'] = 19;
  label_to_char_map['k'] = 20;
  label_to_char_map['l'] = 21;
  label_to_char_map['m'] = 22;
  label_to_char_map['n'] = 23;
  label_to_char_map['o'] = 24;
  label_to_char_map['p'] = 25;
  label_to_char_map['q'] = 26;
  label_to_char_map['r'] = 27;
  label_to_char_map['s'] = 28;
  label_to_char_map['t'] = 29;
  label_to_char_map['u'] = 30;
  label_to_char_map['v'] = 31;
  label_to_char_map['w'] = 32;
  label_to_char_map['x'] = 33;
  label_to_char_map['y'] = 34;
  label_to_char_map['z'] = 35;

  // Reshape according to the first anno_datum of each batch
  // on single input batches allows for inputs of varying dimension.
  const int batch_size = this->layer_param_.data_param().batch_size();
  AnnotatedDatum& anno_datum = *(reader_.full().peek());
  // Use data_transformer to infer the expected blob shape from anno_datum.
  vector<int> top_shape =
      this->data_transformer_->InferBlobShape(anno_datum.datum());
  this->transformed_data_.Reshape(top_shape);
  // Reshape batch according to the batch_size.
  top_shape[0] = batch_size;
  batch->data_.Reshape(top_shape);

  Dtype* top_data = batch->data_.mutable_cpu_data();
  Dtype* top_label = NULL;  // suppress warnings about uninitialized variables
  if (this->output_labels_ && !has_anno_type_) {
    top_label = batch->label_.mutable_cpu_data();
  }

  // Store transformed annotation.
  map<int, vector<AnnotationGroup> > all_anno;
  int num_bboxes = 0;

  for (int item_id = 0; item_id < batch_size; ++item_id) {
    timer.Start();
    // get a anno_datum
    AnnotatedDatum& anno_datum = *(reader_.full().pop("Waiting for data"));
    read_time += timer.MicroSeconds();
    timer.Start();
    AnnotatedDatum sampled_datum;
    if (batch_samplers_.size() > 0) {
      // Generate sampled bboxes from anno_datum.
      vector<NormalizedBBox> sampled_bboxes;
      GenerateBatchSamples(anno_datum, batch_samplers_, &sampled_bboxes);
      if (sampled_bboxes.size() > 0) {
        // Randomly pick a sampled bbox and crop the anno_datum.
        int rand_idx = caffe_rng_rand() % sampled_bboxes.size();
        this->data_transformer_->CropImage(anno_datum, sampled_bboxes[rand_idx],
                                           &sampled_datum);
      } else {
        sampled_datum.CopyFrom(anno_datum);
      }
    } else {
      sampled_datum.CopyFrom(anno_datum);
    }
    // Apply data transformations (mirror, scale, crop...)
    int offset = batch->data_.offset(item_id);
    this->transformed_data_.set_cpu_data(top_data + offset);
    vector<AnnotationGroup> transformed_anno_vec;
    if (this->output_labels_) {
      if (has_anno_type_) {
        // Make sure all data have same annotation type.
        CHECK(sampled_datum.has_type()) << "Some datum misses AnnotationType.";
        CHECK_EQ(anno_type_, sampled_datum.type()) <<
            "Different AnnotationType.";
        // Transform datum and annotation_group at the same time
        transformed_anno_vec.clear();
        this->data_transformer_->Transform(sampled_datum,
                                           &(this->transformed_data_),
                                           &transformed_anno_vec);
        if (anno_type_ == AnnotatedDatum_AnnotationType_BBOX) {
          // Count the number of bboxes.
          for (int g = 0; g < transformed_anno_vec.size(); ++g) {
            num_bboxes += transformed_anno_vec[g].annotation_size();
          }
        } else {
          LOG(FATAL) << "Unknown annotation type.";
        }
        all_anno[item_id] = transformed_anno_vec;
      } else {
        this->data_transformer_->Transform(sampled_datum.datum(),
                                           &(this->transformed_data_));
        // Otherwise, store the label from datum.
        CHECK(sampled_datum.datum().has_label()) << "Cannot find any label.";
        top_label[item_id] = sampled_datum.datum().label();
      }
    } else {
      this->data_transformer_->Transform(sampled_datum.datum(),
                                         &(this->transformed_data_));
    }
    trans_time += timer.MicroSeconds();

    reader_.free().push(const_cast<AnnotatedDatum*>(&anno_datum));
  }

  // Store "rich" annotation if needed.
  if (this->output_labels_ && has_anno_type_) {
    vector<int> label_shape(4);
    if (anno_type_ == AnnotatedDatum_AnnotationType_BBOX) {
      label_shape[0] = 1;
      label_shape[1] = 1;
      label_shape[3] = 8+25;
      if (num_bboxes == 0) {
        // Store all -1 in the label.
        label_shape[2] = 1;
        batch->label_.Reshape(label_shape);
        caffe_set<Dtype>(8+25, -1, batch->label_.mutable_cpu_data());
      } else {
        // Reshape the label and store the annotation.
        label_shape[2] = num_bboxes;
        batch->label_.Reshape(label_shape);
        top_label = batch->label_.mutable_cpu_data();
        int idx = 0;
        for (int item_id = 0; item_id < batch_size; ++item_id) {
          const vector<AnnotationGroup>& anno_vec = all_anno[item_id];
          for (int g = 0; g < anno_vec.size(); ++g) {
            const AnnotationGroup& anno_group = anno_vec[g];
            for (int a = 0; a < anno_group.annotation_size(); ++a) {
              const Annotation& anno = anno_group.annotation(a);
              const NormalizedBBox& bbox = anno.bbox();
              top_label[idx++] = item_id;
              top_label[idx++] = anno_group.group_label();
              top_label[idx++] = anno.instance_id();
              top_label[idx++] = bbox.xmin();
              top_label[idx++] = bbox.ymin();
              top_label[idx++] = bbox.xmax();
              top_label[idx++] = bbox.ymax();
              top_label[idx++] = bbox.difficult();
              top_label[idx++] = anno.img_height();
              top_label[idx++] = anno.img_width();
              
              string word_label = anno.char_label();
              int word_size = word_label.size();
              
              
              for (int c_idx = 0; c_idx < word_size; c_idx++){
                char letter = word_label[c_idx];
                std::map<char,int>::iterator key_test = label_to_char_map.find(tolower(letter));
                if (key_test != label_to_char_map.end()){
                  top_label[idx++] = label_to_char_map[tolower(letter)];
                }else{
                  top_label[idx++] = 36;
                }
              }
              int num_spaces = 23 - word_size;
              for(int space_idx = 0; space_idx < num_spaces; ++space_idx){
                top_label[idx++] = 36;
              }
            }
          }
        }
      }
    } else {
      LOG(FATAL) << "Unknown annotation type.";
    }
  }
  timer.Stop();
  batch_timer.Stop();
  DLOG(INFO) << "Prefetch batch: " << batch_timer.MilliSeconds() << " ms.";
  DLOG(INFO) << "     Read time: " << read_time / 1000 << " ms.";
  DLOG(INFO) << "Transform time: " << trans_time / 1000 << " ms.";
}

INSTANTIATE_CLASS(AnnotatedWordDataLayer);
REGISTER_LAYER_CLASS(AnnotatedWordData);

}  // namespace caffe
