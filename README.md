代码思路总结
总体目标
从视频中逐帧检测红色灯条（典型如 RoboMaster 装甲板灯条），提取其最小外接旋转矩形，记录上下端点并可视化标注。

主流程
1. 读取视频
通过 argc/argv 获取视频路径（命令行参数优先，否则用默认路径 ../assets/output.mp4）

用 cv::VideoCapture 打开，isOpened() 检查是否成功



2. 逐帧处理（while(cap.read(frame))）
每帧依次执行以下步骤：

(1) 颜色空间转换

BGR → HSV，便于用色调（H）分离红色

(2) 红色二值化（双区间合并）

红色在 HSV 中跨越 0/180 边界，故用两段 inRange：

H∈[0,180], S∈[100,255], V∈[80,255]

H∈[160,180], S∈[100,255], V∈[80,255]

bitwise_or 合并两段掩码

(3) 形态学去噪

3×7 矩形核做闭运算（MORPH_CLOSE），填充灯条内部空洞、连接断裂

(4) 轮廓提取

findContours 取外轮廓（RETR_EXTERNAL）+ CHAIN_APPROX_SIMPLE

复制 frame → result 用于画图，保护原帧



3. 轮廓筛选与灯条建模
对每个轮廓：

用 minAreaRect 求最小外接旋转矩形

将宽高统一为 宽<高（std::swap），长边即灯条长度 h

筛选规则：

h > max_h * 0.4（保留接近本帧最长的轮廓）

area > 20 && h > 10（剔除噪点）

通过筛选的轮廓封装为 LightBar：

把旋转矩形的 4 个顶点按 y 坐标排序

top = (pts[0]+pts[1])/2（上两点中点）

bottom = (pts[2]+pts[3])/2（下两点中点）

保存 rect 与 top/bottom，push_back 到 bars



4. 可视化
用 draw_order = {0,1,3,2} 连接四点画绿色矩形（避免交叉连线）

用 boundingRect 定位于轮廓旁标注 W:.. H:..，靠上边缘时改到下方防止出界

imshow 显示 Result（原图）与 Result2（标注图）

waitKey 兼具帧间延时与 ESC 退出判断
