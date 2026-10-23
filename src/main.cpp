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
    
    
    /*while(cap.read(frame)){              //将cap里的视频逐帧存给frame，如果无照片可存，返回false，循环不执行
        if(frame.empty()) break;         //检查措施，如果frame是空的，不执行
        cv::imshow("raw",frame);         //在名为raw的窗口播放存在frame的视频（逐帧照片）
        if(cv::waitKey(1000) == 27) break;//防止白屏，给opencv窗口喘息时间（1mm或其他可调时长），顺便检查是否按esc退出
    }*/
    //————————————二值化+掩码 ==> 初步突出目标————————————
    cv::Mat hsv, mask1, mask2, mask;
    /*cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);//“BGR”格式图片转换为（to/2）HSV格式图片，（目标相框，目标格式，转换方法）
    cv::inRange(hsv,cv::Scalar(0,43,46),cv::Scalar(180,255,255),mask1);
    cv::inRange(hsv,cv::Scalar(160,43,46),cv::Scalar(180,255,255),mask2);
    cv::bitwise_or(mask1,mask2,mask);//将两段红色掩码拼图在一起*/
    
    //————————————形态学去噪 ==> 进一步突出目标(膨胀目标，腐蚀噪点)————————————
    /*cv::Mat kernal = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3));
    cv::morphologyEx(mask,mask,cv::MORPH_OPEN,kernal);*/
    
    
    
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
        std::vector<std::vector<cv::Point>> contours;//定义一个二维数组，存放轮廓像素点
        cv::findContours(mask, contours/*输出形式*/, cv::RETR_EXTERNAL/*只要外轮廓*/, cv::CHAIN_APPROX_SIMPLE/*只要外轮廓四个点*/);
        cv::Mat result = frame.clone(); // 用于画图展示//保护frame数据，用于对比
        
        
        
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
            float rect_h = rotRect.size.height;
            
            if (rect_w > rect_h) {
                std::swap(rect_w, rect_h); 
            }

            int w = static_cast<int>(rect_w);
            int h = static_cast<int>(rect_h);
            
            if (h < max_h * 0.4) continue;    
            if(area < 20 || h < 10) continue; 
            
            std::cout << "轮廓 " << i << " - 宽: " << w << ", 高(长度): " << h << ", 面积: " << area << std::endl;
            
            // 4. 绘制旋转矩形（提取4个顶点画线）
            cv::Point2f pts[4];//创建了一个装“点”的空盒子
            rotRect.points(pts);//根据旋转矩形的角度，宽高，计算四点坐标存入上述空盒子
            std::sort(pts,pts+4,[](const cv::Point2f& a,const cv::Point2f& b){return a.y < b.y ||(a.x < b.x && a.y == b.y);});
            
                    
            LightBar lb;
            lb.rect = rotRect;
            lb.top = (pts[0] + pts[1]) * 0.5f;
            lb.bottom = (pts[2] + pts[3]) * 0.5f;                          
            
            bars.push_back(lb);
            
            
            //根据点坐标连线
            int draw_order[4] = {0, 1, 3, 2}; 
            for (int j = 0; j < 4; j++) {
                cv::line(result, pts[draw_order[j]], pts[draw_order[(j + 1) % 4]], cv::Scalar(0, 255, 0), 1);
            }

            //借助正矩形进行文字定位
            cv::Rect bRect = cv::boundingRect(contours[i]);
            std::string text = "W:" + std::to_string(w) + " H:" + std::to_string(h);
            cv::Point text_pos(bRect.x - 10, bRect.y - 5); 
            if (text_pos.y < 10)  text_pos.y = bRect.y + bRect.height + 15; 
            cv::putText(result, text, text_pos, cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
        }


        

        
        
        
        
        
    
        cv::imshow("Result", frame);      //在名为Result的窗口播放存在frame的视频（逐帧照片）
        cv::imshow("Result2", result);
        if(cv::waitKey(1000) == 27) break;//防止白屏，给opencv窗口喘息时间（1mm或其他可调时长），顺便检查是否按esc退出
    }



    return 0;
}