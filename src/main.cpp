#include<opencv2/opencv.hpp>
#include<iostream>
#include <vector>
#include <algorithm>

int main(int argc,char** argv){
    //————————————获取并播放视频————————————
    std::string videoPath = (argc > 1) ? argv[1] : "../assets/output.mp4";//获取视频路径，用于cap读取视频//argc+argv工作原理有疑惑
    cv::VideoCapture cap(videoPath);     //cap（capture）是一个自定义的名字，创建了一个名叫cap的视频视频读取机
    if (!cap.isOpened()) {               //cap.isOpened() == “打开了”用于检查视频是否成功打开（例如文件是否存在、格式是否支持）。
        std::cerr << "无法打开视频: " << videoPath << std::endl;
        return -1;
    }
    cv::Mat frame;                       //声明矩阵Mat对象frame（帧）

    
    
    const cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        9.28130989e+02, 0.0, 3.77572945e+02,
        0.0, 9.30138391e+02, 2.83892859e+02,
        0.0, 0.0, 1.0);//3*3矩阵，保存相机内参K矩阵
    const cv::Mat distCoeffs = (cv::Mat_<double>(1, 5) <<
        -2.54433647e-01, 5.69431382e-01, 3.65405229e-03,
        -1.09433818e-03, -1.33846840e+00);//1*5矩阵，保存相机畸变参数
    constexpr float plateWidth = 130.0f;
    constexpr float plateHeight = 50.0f;
    constexpr float axisLength = 200.0f;//`axisLength` 是绘制坐标轴的实际长度，也为 **mm**
    
    // FLU: +X points out of the armor plate, +Y points left, and +Z points up.
    const std::vector<cv::Point3f> plateCorners = {
        cv::Point3f(0.0f, plateWidth / 2.0f, plateHeight / 2.0f),
        cv::Point3f(0.0f, -plateWidth / 2.0f, plateHeight / 2.0f),
        cv::Point3f(0.0f, -plateWidth / 2.0f, -plateHeight / 2.0f),
        cv::Point3f(0.0f, plateWidth / 2.0f, -plateHeight / 2.0f)
    };
    
    
    
    
    cv::Mat hsv, mask1, mask2, mask;
    while(cap.read(frame)){ 
        if(frame.empty()) break;        //检查措施，如果frame是空的，不执行
        
        //————————————二值化+掩码 ==> 初步突出目标————————————
        cv::Mat hsv, mask1, mask2, mask;//将cap里的视频逐帧存给frame，如果无照片可存，返回false，循环不执行
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);//“BGR”格式图片转换为（to/2）HSV格式图片，（目标相框，目标格式，转换方法）
        cv::inRange(hsv,cv::Scalar(0,100,80),cv::Scalar(180,255,255),mask1);
        cv::inRange(hsv,cv::Scalar(160,100,80),cv::Scalar(180,255,255),mask2);
        cv::bitwise_or(mask1,mask2,mask);
        
        //————————————形态学去噪 ==> 进一步突出目标(膨胀目标，腐蚀噪点)————————————
        cv::Mat kernal = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,7));
        cv::morphologyEx(mask,mask,cv::MORPH_CLOSE,kernal);
        cv::imshow("Morphology_Mask", mask);
        
        //————————————寻找轮廓————————
        std::vector<std::vector<cv::Point>> contours;//定义一个二维数组类型的容器（contours），存放轮廓像素点
        cv::findContours(mask, contours/*输出形式*/, cv::RETR_EXTERNAL/*只要外轮廓*/, cv::CHAIN_APPROX_SIMPLE/*只要外轮廓四个点*/);
        cv::Mat result = frame.clone();              // 用于画图展示//保护frame数据，用于对比展示效果

        
        struct LightBar{
            cv::Point2f top;      // 灯条顶部的中心点
            cv::Point2f bottom;   // 灯条底部的中心点
            cv::RotatedRect rect; // 灯条的最小外接旋转矩形
            };
        std::vector<LightBar> bars;
        
        
        
        int max_h = 0;
        for(size_t i = 0;i < contours.size();i++){
            cv::RotatedRect rotRect = cv::minAreaRect(contours[i]);
            int current_h = static_cast<int>(std::max(rotRect.size.width, rotRect.size.height));//取旋转矩形中的较长边作为“长边”
            if(current_h > max_h){
                max_h = current_h;
            }
            double area = cv::contourArea(contours[i]);
            float rect_w = rotRect.size.width;
            float rect_h = rotRect.size.height;//？？？？？？为什么是浮点型？？？？？？
            
            if (rect_w > rect_h) {
                std::swap(rect_w, rect_h);    //长宽转换，始终保持长的边是h，短的是w
            }

            int w = static_cast<int>(rect_w); //float ==> int 进行大小比较
            int h = static_cast<int>(rect_h);
            if (h < max_h * 0.4) continue;    
            if(area < 20 || h < 10) continue; //筛掉干扰信息
            
            std::cout << "轮廓 " << i << " - 宽: " << w << ", 高(长度): " << h << ", 面积: " << area << std::endl;
            
            // 4. 绘制旋转矩形（提取4个顶点画线）
            cv::Point2f pts[4];  //创建了一个装“点”的空盒子
            rotRect.points(pts); //根据旋转矩形的角度，宽高，计算四点坐标存入上述空盒子
            std::sort(pts,pts+4,[](const cv::Point2f& a,const cv::Point2f& b){return a.y < b.y ||(a.x < b.x && a.y == b.y);});
            
                    
            LightBar lb;         //“LightBar”类型的结构体 lb 充当送货员，将一根灯条上下端点的坐标存放到bars中
            lb.rect = rotRect;
            lb.top = (pts[0] + pts[1]) * 0.5f;
            lb.bottom = (pts[2] + pts[3]) * 0.5f;                          
            
            bars.push_back(lb);
            
            
            //借助正矩形进行文字定位
            cv::Rect bRect = cv::boundingRect(contours[i]);
            std::string text = "W:" + std::to_string(w) + " H:" + std::to_string(h);
            cv::Point text_pos(bRect.x - 10, bRect.y - 5); 
            if (text_pos.y < 10)  text_pos.y = bRect.y + bRect.height + 15; 
            cv::putText(result, text, text_pos, cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
        }


        // 按灯条中心的横坐标从左到右排序，便于将相邻灯条配对。
        std::sort(bars.begin(), bars.end(), [](const LightBar& a, const LightBar& b) {
            return a.rect.center.x < b.rect.center.x;
        });
        // 每两根灯条作为一组，用上下端点组成矩形的四个顶点。
        for (size_t i = 0; i + 1 < bars.size(); i += 2) {
        // 用四条直线依次连接灯条端点，绘制绿色闭合矩形。
            cv::line(result, bars[i].top, bars[i + 1].top, cv::Scalar(0, 255, 0), 2);
            cv::line(result, bars[i + 1].top, bars[i + 1].bottom, cv::Scalar(0, 255, 0), 2);
            cv::line(result, bars[i + 1].bottom, bars[i].bottom, cv::Scalar(0, 255, 0), 2);
            cv::line(result, bars[i].bottom, bars[i].top, cv::Scalar(0, 255, 0), 2);

            const std::vector<cv::Point2f> imageCorners = {
                bars[i].top,
                bars[i + 1].top,
                bars[i + 1].bottom,
                bars[i].bottom
            };
            cv::Mat rvec, tvec;
            if (cv::solvePnP(plateCorners, imageCorners, cameraMatrix, distCoeffs,
                             rvec, tvec, false, cv::SOLVEPNP_IPPE) &&
                tvec.at<double>(2, 0) > 0.0) {
                const std::vector<cv::Point3f> axes = {
                    cv::Point3f(0.0f, 0.0f, 0.0f),
                    cv::Point3f(axisLength, 0.0f, 0.0f),
                    cv::Point3f(0.0f, -axisLength, 0.0f),
                    cv::Point3f(0.0f, 0.0f, axisLength)
                };
                std::vector<cv::Point2f> projectedAxes;
                cv::projectPoints(axes, rvec, tvec, cameraMatrix, distCoeffs, projectedAxes);

                const cv::Point origin(
                    cvRound(projectedAxes[0].x), cvRound(projectedAxes[0].y));
                cv::arrowedLine(result, origin, projectedAxes[1], cv::Scalar(0, 0, 255), 2);
                cv::arrowedLine(result, origin, projectedAxes[2], cv::Scalar(0, 255, 0), 2);
                cv::arrowedLine(result, origin, projectedAxes[3], cv::Scalar(255, 0, 0), 2);
                cv::putText(result, "X(F)", projectedAxes[1], cv::FONT_HERSHEY_SIMPLEX,
                            0.45, cv::Scalar(0, 0, 255), 1);
                cv::putText(result, "Y(L)", projectedAxes[2], cv::FONT_HERSHEY_SIMPLEX,
                            0.45, cv::Scalar(0, 255, 0), 1);
                cv::putText(result, "Z(U)", projectedAxes[3], cv::FONT_HERSHEY_SIMPLEX,
                            0.45, cv::Scalar(255, 0, 0), 1);

                const std::string positionText =
                    "t_cam(mm): x=" + std::to_string(tvec.at<double>(0, 0)) +
                    " y=" + std::to_string(tvec.at<double>(1, 0)) +
                    " z=" + std::to_string(tvec.at<double>(2, 0));
                cv::putText(result, positionText,
                            cv::Point(10, 25 + static_cast<int>(i / 2) * 20),
                            cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);
            }
        }

        
        
        
        
        
    
        cv::imshow("Result", frame);      //在名为Result的窗口播放存在frame的视频（逐帧照片）
        cv::imshow("Result2", result);
        if(cv::waitKey(1000) == 27) break;//防止白屏，给opencv窗口喘息时间（1mm或其他可调时长），顺便检查是否按esc退出
    }



    return 0;
}