<img src="GraSU_free-file.png" width="16%" height="16%">

# GraSU: A Fast Graph Update Library for FPGA-based Dynamic Graph Processing

## Introduction

GraSU is a graph-structured update library for high-throughput dynamic graph updates on FPGA. GraSU can be easily integrated with any existing FPGA-based static graph accelerators for handling dynamic graphs. We implement GraSU on Xilinx Alveo U250 FPGA card (real hardware).

## Prerequisites

### Hardware

This project works on [Xilinx Alveo U250 Data Center Accelerator Card](https://www.xilinx.com/products/boards-and-kits/alveo/u250.html).

### Operating System

Ubuntu 18.04 LTS

### Software

[Vitis Core Development Kit 2020.2](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis/2020-2.html)

[Alveo U250 Package File on Vitis 2020.2](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/alveo/u250.html)

## Run the code

1. Install the software development environment according to [Xilinx documentation](https://www.xilinx.com/support/documentation/boards_and_kits/accelerator-cards/1_9/ug1301-getting-started-guide-alveo-accelerator-cards.pdf). After installation, you can use commands

```
sudo /opt/xilinx/xrt/bin/xbmgmt flash --scan
/opt/xilinx/xrt/bin/xbutil validate
```

to make sure that the runtime environment and the alveo card is ready.

2. Add the xrt and Vitis to your PATH. Typically you can run

```
source /opt/xilinx/xrt/setup.sh
source /tools/Xilinx/Vitis/2020.2/settings64.sh
```

3. Clone, build and run

``` sh
$ git clone https://github.com/qgwang-hust/GraSU.git
$ cd GraSU/GraSU/Hardware
$ make all          # build the host program
$ cd GraSU/GraSU_kernels/Hardware
$ make -j24 all     # build the hardware kernels, you can specify the number of compilation threads according to your CPU cores
$ cd GraSU/GraSU_system_hw_link/Hardware
$ make -j24 all     # link the kernel objects with the hardware platform XSA file to produce the device binary XCLBIN file
$ cd GraSU/GraSU_system/Hardware
$ make all          # package the system

$ cd GraSU/GraSU/Hardware
$ ./GraSU ./GraSU.xclbin <graph_file> <result_file>     # run the program
```


## Related publications

If you use GraSU, please cite our research paper published at FPGA 2021.

**\[FPGA'21\]** Qinggang Wang, Long Zheng, Yu Huang, Pengcheng Yao, Chuangyi Gui, Xiaofei Liao, Hai Jin, Wenbin Jiang, Fubing Mao, "[GraSU: A Fast Graph Update Library for FPGA-based Dynamic Graph Processing](https://dl.acm.org/doi/10.1145/3431920.3439288)", in Proceedings of the 2021 ACM/SIGDA International Symposium on Field Programmable Gate Arrays (FPGA'21), 2021. 

```javascript
@inproceedings{DBLP:conf/fpga/Wang00YGL0JM21,
  author    = {Qinggang Wang and Long Zheng and Yu Huang and Pengcheng Yao and Chuangyi Gui and Xiaofei Liao and Hai Jin and Wenbin Jiang and Fubing Mao},
  title     = {GraSU: A Fast Graph Update Library for FPGA-based Dynamic Graph Processing},
  booktitle = {Proceedings of the 2021 ACM/SIGDA International Symposium on Field Programmable Gate Arrays (FPGA'21)},
  pages     = {149--159},
  publisher = {ACM},
  year      = {2021},
  url       = {https://doi.org/10.1145/3431920.3439288},
}
```

## License & Copyright of GraSU
GraSU is implemented by Qinggang Wang and [Ao Hu](https://github.com/pauvrepetit) at Cluster and Grid Computing Lab & Services Computing Technology and System Lab in Huazhong University of Science and Technology([HUST SCTS & CGCL Lab](http://grid.hust.edu.cn/)), the copyright of this GraSU remains with CGCL & SCTS Lab of Huazhong University of Science and Technology.
