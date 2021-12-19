#ifndef PMA_DYNAMIC_GRAPH_H
#define PMA_DYNAMIC_GRAPH_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#include "kernel_config.h"

const unsigned int COL_IDX_NONE = -1;
bool cmp(std::pair<unsigned int, double> a, std::pair<unsigned int, double> b);

class pma_dynamic_graph {
  public:
    unsigned int node_size;
    const int segment_size = SEGMENT_SIZE;
    std::vector<unsigned long> data;          // PMA Array
    std::vector<unsigned long> row_offset;    // 每个顶点开始的位置，我们把长度定为node_size + 1吧，这样我们就不必用到INF的边做为边界了
    std::vector<unsigned long> binary_search; // 记录二分查找所需的数据

    std::vector<unsigned int> node_map; // 顶点重排时的id映射关系，顶点i映射到顶点node_map[i]

  private:
    // 根据顶点作为源节点的更新次数对顶点进行重排
    // 更新次数最多的顶点，重排后id更小，在PMA数组上位置更靠前
    void reorder(unsigned long *update_edges, size_t update_edge_size, unsigned long *init_edges, size_t init_edge_size) {
        std::vector<unsigned int> update_count;     // 记录每个顶点作为源节点的更新次数
        std::vector<unsigned int> segment_count;    // 记录每个顶点作为源顶点在pma数组中的段的数量
        std::vector<double> update_per_segment;     // 记录每个顶点作为源顶点时，在每个段上的平均更新次数，平均值越大，该顶点对应的段价值越大
        update_count.resize(node_size);
        segment_count.resize(node_size);
        update_per_segment.resize(node_size);
        for (unsigned int i = 0; i < node_size; i++) {
            update_count[i] = 0;
            segment_count[i] = 0;
        }
        for (size_t i = 0; i < update_edge_size; i++)
            update_count[(update_edges[i] & ~EMPTY) >> 32]++;
        
        // 对init_edges和update_edges做一次合并，才能统计每个顶点所占用的段的数量
        std::vector<unsigned long> tmp_edges(init_edges, init_edges + init_edge_size);
        for (size_t i = 0; i < update_edge_size; i++)
            if ((update_edges[i] >> 63) == 0)
                tmp_edges.push_back(update_edges[i]);
        std::sort(tmp_edges.begin(), tmp_edges.end());
        tmp_edges.erase(std::unique(tmp_edges.begin(), tmp_edges.end()), tmp_edges.end());
        for (size_t i = 0; i < tmp_edges.size(); i++) {
            segment_count[tmp_edges[i] >> 32]++;
        }

        for (size_t i = 0; i < node_size; i++) {
            if (segment_count[i] % 16 != 0) {
                segment_count[i] = segment_count[i] / 16 + 1;
            } else {
                segment_count[i] = segment_count[i] / 16;
            }
        }

        for (size_t i = 0; i < node_size; i++) {
            if (segment_count[i] == 0) {
                update_per_segment[i] = -1;
            } else {
                update_per_segment[i] = update_count[i] * 1.0 / segment_count[i];
            }
        }
        
        std::vector<std::pair<unsigned int, double>> count;
        for (unsigned int i = 0; i < node_size; i++)
            count.push_back(std::make_pair(i, update_per_segment[i]));
        std::sort(count.begin(), count.end(), cmp);

        node_map.resize(node_size);
        // count[0].first这个顶点的更新次数是最多的，那么我们将其映射成顶点0，以此类推
        for (unsigned int i = 0; i < node_size; i++)
            node_map[count[i].first] = i;
    }

    // 将edges中的顶点id进行映射
    void map_node(unsigned long *edges, size_t edge_size) {
        for (size_t i = 0; i < edge_size; i++) {
            unsigned long del = edges[i] & EMPTY;
            unsigned long edge = edges[i] & ~EMPTY;
            unsigned long begin_node = edge >> 32;
            unsigned long end_node = edge & 0xFFFFFFFFUL;
            begin_node = node_map[begin_node];
            end_node = node_map[end_node];
            edges[i] = del + (begin_node << 32) + end_node;
        }
    }

  public:
    pma_dynamic_graph(int num_nodes, unsigned long *init_edges, size_t init_edge_size, unsigned long *update_edges, size_t update_edge_size) : node_size(num_nodes) {
        reorder(update_edges, update_edge_size, init_edges, init_edge_size);
        map_node(init_edges, init_edge_size);
        map_node(update_edges, update_edge_size);

        std::vector<unsigned long> all_edges(init_edges, init_edges + init_edge_size);
        for (size_t i = 0; i < update_edge_size; i++)
            if ((update_edges[i] >> 63) == 0)
                all_edges.push_back(update_edges[i]);
        std::sort(all_edges.begin(), all_edges.end());
        all_edges.erase(std::unique(all_edges.begin(), all_edges.end()), all_edges.end());  // 去重

        row_offset.resize(node_size + 1);
        row_offset[0] = 0;
        int cur_node = 0;
        int cur_count = 0;
        int cur_loc = 0;
        for (size_t i = 0; i < node_size; i++) {
            while ((all_edges[cur_loc] >> 32) == i) {
                cur_count++;
                cur_loc++;
            }
            if (cur_count % 16 != 0) {
                cur_count = cur_count / 16 * 16 + 16;
            }
            row_offset[i + 1] = row_offset[i] + cur_count;
            cur_count = 0;
        }

        data.resize(row_offset[node_size]);
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = EMPTY;
        }
        cur_node = 0;
        cur_loc = 0;
        for (size_t i = 0; i < all_edges.size(); i++) {
            if ((all_edges[i] >> 32) == cur_node) {
                data[cur_loc++] = all_edges[i];
            } else {
                cur_node = all_edges[i] >> 32;
                cur_loc = row_offset[cur_node];
                data[cur_loc++] = all_edges[i];
            }
        }

        binary_search.resize(row_offset[node_size] / 16);
        for (size_t i = 0; i < binary_search.size(); i++) {
            binary_search[i] = data[i * 16];
        }
        // 最后我们要把data中尚未添加进来的边删掉
        std::vector<unsigned long> init_edges_vector(init_edges, init_edges + init_edge_size);
        std::sort(init_edges_vector.begin(), init_edges_vector.end());
        cur_loc = 0;
        for (size_t i = 0; i < data.size(); i++) {
            if (data[i] == init_edges_vector[cur_loc]) {
                cur_loc++;
            } else {
                data[i] = EMPTY;
            }
        }
        for (size_t i = 0; i < data.size(); i += 16) {
            cur_loc = 0;
            for (size_t j = 0; j < 16; j++) {
                if (data[i + j] != EMPTY) {
                    data[i + cur_loc] = data[i + j];
                    cur_loc++;
                }
            }
            for (size_t j = cur_loc; j < 16; j++) {
                data[i + j] = EMPTY;
            }
        }
    }
};

#endif //PMA_DYNAMIC_GRAPH_H
