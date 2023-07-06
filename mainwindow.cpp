#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <vector>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* 为了将图像能在GUI上显示，将opencv里Mat类型的图像转为QT里QImage类型 */
QImage MainWindow::MattoQImage(const cv::Mat &mat)
{
    switch(mat.type())//判断图像类型
    {
    case CV_8UC3://当图像是3通道8位无符号整形型时（彩色图）
    {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    case CV_8UC1://当图像是单通道8位无符号整形型时（灰度图）
    {
        QVector<QRgb>  color_table;
        if(color_table.isEmpty())
        {
            for(int i = 0; i != 256; ++i)
                color_table.push_back(qRgb(i, i, i));
        }
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Indexed8);
        image.setColorTable(color_table);
        return image;
    }
    default://当图像的类型不是以上两种时
        ui->labframe->setText("图像类型不支持！！！");
    }
    return QImage();//返回默认构造的图像
}

/* 米粒计数函数 */
int MainWindow::count(qreal &aver_area)
{
    int value = ui->sboxthreshold->value();//获取设定的阈值
    rice_pionts.resize(0);//将米粒数重置为0
    cv::Mat bin_img;
    if(ui->rbtnThres->isChecked())//假如阈值是自动调整的
    {
        value = cv::threshold(gray_img, bin_img, 0, 255, cv::THRESH_OTSU);//使用大津法来调整阈值，二值化
        ui->sboxthreshold->setValue(value);//将调整后的值设置到sboxthreshold
    }
    else//假如阈值是手动调整的
        cv::threshold(gray_img, bin_img, value, 255, cv::THRESH_BINARY);//按照设定的值，二值化
    //为下面的形态学开操作准备一个结构元素
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(bin_img, bin_img, cv::MORPH_OPEN, element);//执行形态学开操作
    std::vector<std::vector<cv::Point>> contours;//存放找到的所有闭形区域（可能的米粒）
    cv::findContours(bin_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);//寻找图像中闭形区域
    aver_area = 0;//重置平均面积为零，此变量是为了下一步缺陷分析准备
    for(const auto &cont : contours)//遍历所有闭形区域
    {
        qreal area = cv::contourArea(cont);//得到闭形区域面积
        if(area > 100)//面积大于100
        {
            aver_area += area;
            rice_pionts.push_back(cont);//则是米粒，放入存放米粒的vector中
        }
    }
    aver_area /= rice_pionts.size();//计算平均面积
    return rice_pionts.size();//返回米粒个数
}

/* 缺陷分析函数 */
int MainWindow::defectAnalysis(cv::Mat &result_img, qreal aver_area)
{
    size_t i = 0, good_rice = 0;
    for(const auto &rp : rice_pionts)//遍历所有米粒
    {
        cv::Scalar color;
        cv::String text;
        cv::Point pre_point = rp.front();//得到该米粒的坐标
        cv::Point2f potf[4];
        cv::RotatedRect rrt(cv::minAreaRect(rp));//求该米粒的最小外接矩形
        //得到矩形的最长边和最小边的比值
        qreal rate = (rrt.size.width > rrt.size.height) ? (rrt.size.width / rrt.size.height) :
                     (rrt.size.height / rrt.size.width);
        //当面积小于平均面积的0.75或比值小于1.58，则认为有缺陷
        if(rate < 1.58 || (cv::contourArea(rp) < (aver_area * 0.75)))
            color = cv::Scalar(0, 0, 255);//将米粒的轮廓设置为红色
        else//否则，认为无缺陷
        {
            ++good_rice;//无缺陷米计数增1
            color = cv::Scalar(0, 255, 0);//将米粒的轮廓设置为绿色
        }
        rrt.points(potf);//得到矩形的四个顶点
        /* 画出矩形，线条粗细设置为2 */
        cv::line(result_img, potf[0], potf[1], color, 2);
        cv::line(result_img, potf[1], potf[2], color, 2);
        cv::line(result_img, potf[2], potf[3], color, 2);
        cv::line(result_img, potf[3], potf[0], color, 2);
        //标明米粒的序号
        cv::putText(result_img, std::to_string(++i), pre_point, cv::FONT_HERSHEY_COMPLEX, 0.6, color, 2);
        //        std::cerr << i << ":" << rate << "\t" << cv::contourArea(rp) << std::endl;
    }
    return good_rice;//返回无缺陷米的数量
}

/* 槽函数，点击计数按钮开始技术及缺陷分析 */
void MainWindow::on_btnStart_clicked()
{
    qreal aver_area;
    cv::Mat result_img = frame.clone();//得到一份原图的副本，在其上绘制结果
    int total_rice = count(aver_area);//得到全部米粒数
    //    std::cerr << aver_area;
    //进行缺陷分析，得到结果图和无缺陷米的数量
    int good_rice = defectAnalysis(result_img, aver_area);
    /* 得到绘图窗口大小 */
    int w = ui->labBin->width();
    int h = ui->labBin->height();
    cv::resize(result_img, result_img, cv::Size(w, h));//将图象放缩到窗口大小
    /* 将米粒总数，无缺陷米粒数，有缺陷米粒数分别设置到指定的模块 */
    ui->lcdRiceNum->display(total_rice);
    ui->lcdGood->display(good_rice);
    ui->lcdBad->display(total_rice - good_rice);
    ui->labBin->setPixmap(QPixmap::fromImage(MattoQImage(result_img)));//将图像绘制在结果窗口上
}

/* 槽函数，点击读取图像按钮从摄像头获取图像并显示 */
void MainWindow::on_btnRead_clicked()
{
    cv::Mat temp_img;
    /* 得到绘图窗口大小 */
    int w = ui->labframe->width();
    int h = ui->labframe->height();
    QString str = ui->edtImg->text();//获取摄像头的端口号或IP地址
    /* 打开摄像头 */
    if(!str[0].isDigit())
        cap.open(str.toStdString());
    else
        cap.open(str.toInt());
    if(!cap.isOpened())//如果打开失败，进行提示
        ui->labframe->setText("摄像头打开失败,请重试...");
    else//否则
    {
        cap >> frame;//读取一帧图像
        cap.release();//关闭摄像头
        //此处进行截取是因为所使用的网络摄像头的软件在传输图像时会带有水印，为了去掉水印
        frame = frame(cv::Rect(0, 15, 640, 480 - 15));
        cv::cvtColor(frame, gray_img, cv::COLOR_RGB2GRAY);//将图像转换为灰度，位米粒计数作准备
        cv::resize(frame, temp_img, cv::Size(w, h));//将图象放缩到窗口大小
        ui->labframe->setPixmap(QPixmap::fromImage(MattoQImage(temp_img)));//将图像绘制在原图窗口上
    }
}
/* 槽函数，图像二值化阈值调节*/
void MainWindow::on_sboxthreshold_valueChanged(int arg1)
{
    cv::Mat bin_img;
    if(ui->rbtnThres->isChecked())//假如阈值是自动调整的
    {
        arg1 = cv::threshold(gray_img, bin_img, 0, 256, cv::THRESH_OTSU);//得到调整后的阈值
        ui->sboxthreshold->setValue(arg1);//设置到sboxthreshold上
    }
    else//假如阈值是手动调整的
    {
        cv::threshold(gray_img, bin_img, arg1, 256, cv::THRESH_BINARY);//将图像二值化
        /* 得到绘图窗口大小 */
        int w = ui->labBin->width();
        int h = ui->labBin->height();
        cv::resize(bin_img, bin_img, cv::Size(w, h), 0, 0);//将图象放缩到窗口大小
        ui->labBin->setPixmap(QPixmap::fromImage(MattoQImage(bin_img)));//将图像绘制在结果窗口上
    }
}
