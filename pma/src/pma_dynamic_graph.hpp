#ifndef PMA_DYNAMIC_GRAPH_H
#define PMA_DYNAMIC_GRAPH_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

const unsigned int COL_IDX_NONE = -1;

class pma_dynamic_graph {
  public:
    int node_size;
    const int segment_size = 16;
    std::vector<unsigned long> data;          // PMA Array
    std::vector<unsigned long> row_offset;    // 每个顶点开始的位置，我们把长度定为node_size + 1吧，这样我们就不必用到INF的边做为边界了
    std::vector<unsigned long> binary_search; // 记录二分查找所需的数据

  public:
    pma_dynamic_graph(int num_nodes, unsigned long *init_edges, size_t init_edge_size, unsigned long *update_edges, size_t update_edge_size) {
        node_size = num_nodes;

        std::vector<unsigned long> all_edges;
        all_edges.resize(0);
        for (size_t i = 0; i < init_edge_size; i++) {
            all_edges.push_back(init_edges[i]);
        }
        for (size_t i = 0; i < update_edge_size; i++) {
            if ((update_edges[i] >> 63) == 0)
                all_edges.push_back(update_edges[i]);
        }
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
            data[i] = 0x8000000000000000UL;
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
            if (binary_search[i] == 0x8000000000000000UL) {
                printf("error data[%x] == EMPTY\n", i * 16);
            }
        }
        // 最后我们要把data中尚未添加进来的边删掉
        std::vector<unsigned long> init_edges_vector(init_edges, init_edges + init_edge_size);
        std::sort(init_edges_vector.begin(), init_edges_vector.end());
        cur_loc = 0;
        for (size_t i = 0; i < data.size(); i++) {
            if (data[i] == init_edges_vector[cur_loc]) {
                cur_loc++;
            } else {
                data[i] = 0x8000000000000000UL;
            }
        }
        for (size_t i = 0; i < data.size(); i += 16) {
            cur_loc = 0;
            for (size_t j = 0; j < 16; j++) {
                if (data[i + j] != 0x8000000000000000UL) {
                    data[i + cur_loc] = data[i + j];
                    cur_loc++;
                }
            }
            for (size_t j = cur_loc; j < 16; j++) {
                data[i + j] = 0x8000000000000000UL;
            }
        }
    }
};

#endif //PMA_DYNAMIC_GRAPH_H
