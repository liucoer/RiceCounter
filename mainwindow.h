#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnStart_clicked();
    void on_btnRead_clicked();
    void on_sboxthreshold_valueChanged(int arg1);

private:
    Ui::MainWindow *ui;
    cv::VideoCapture cap;//摄像头
    cv::Mat frame;//原图
    cv::Mat gray_img;//灰度图
    std::vector<std::vector<cv::Point>> rice_pionts;//所有米粒
    QImage MattoQImage(const cv::Mat &mat);//Mat转QImage
    int count(qreal &aver_area);//米粒计数
    int defectAnalysis(cv::Mat &result_img, qreal aver_area);//缺陷分析
};
#endif // MAINWINDOW_H
