- `main.cpp`：主程序，负责读取图片、调用检测接口、终端打印信息及绘制最终结果图。
- `armor_detection.cpp` / `armor_detection.h`：核心算法实现（包含 HSV 分割、形态学、轮廓查找、几何配对、坐标排序）。
- `images/`：存放测试原图 `outpost_frame.png`。
- `output/`：存放程序运行生成的 mask、灯条候选图和最终结果图。

- 在当前目录下打开终端，执行以下命令进行编译：
  ```bash
  cmake -S . -B build -G "MinGW Makefiles"
  cmake --build build
- 运行程序时需通过命令行传入图片路径：
  ```bash
  .\build\armor_detection.exe images/outpost_frame.png
  
