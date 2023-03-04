/*
  Copyright 2016 Lucas Walter
*/

#include "rqt_virtual_dbus/dbus_plugin.h"
#include <QStringList>
#include <pluginlib/class_list_macros.h>

namespace rqt_virtual_dbus {

MyPlugin::MyPlugin() : rqt_gui_cpp::Plugin(), widget_(nullptr) {
  // Constructor is called first before initPlugin function, needless to say.

  // give QObjects reasonable names
  setObjectName("MyPlugin");
}

void MyPlugin::initPlugin(qt_gui_cpp::PluginContext &context) {
  // access standalone command line arguments
  QStringList argv = context.argv();
  // create QWidget
  widget_ = new QWidget();
  // extend the widget with all attributes and children from UI file
  ui_.setupUi(widget_);
  // add widget to the user interface
  if (context.serialNumber() > 1)
    widget_->setWindowTitle(widget_->windowTitle() +
                            (' (%d)' % context.serialNumber()));
  context.addWidget(widget_);

  ui_.topicLineEdit->setText("/dbus");
  topic_name_ = ui_.topicLineEdit->text();
  joy_stick_left_ = new JoyStick(ui_.joy);
  joy_stick_right_ = new JoyStick(ui_.joy_3);
  slip_button_ = new SlipButton(ui_.slipButton);

  pub_rate_ = ui_.rateSpinBox->value();
  pub_timer_ = new QTimer(this);
  topicNameUpdated();

  connect(this->joy_stick_left_, &JoyStick::pointMoved, this,
          &MyPlugin::getLeftJoyValue);
  connect(this->joy_stick_right_, &JoyStick::pointMoved, this,
          &MyPlugin::getRightJoyValue);
  connect(ui_.topicLineEdit, &QLineEdit::editingFinished, this,
          &MyPlugin::topicNameUpdated);
  connect(ui_.rateSpinBox, &QSpinBox::editingFinished, this,
          &MyPlugin::publishRateSpinBoxChanged);
  connect(this->pub_timer_, &QTimer::timeout, this, &MyPlugin::updatePublisher);
  connect(this->pub_timer_, &QTimer::timeout, this,
          &MyPlugin::updateROSPublishState);
  connect(this->slip_button_, &SlipButton::stateChanged, this,
          &MyPlugin::slipButtonChanged);
}

void MyPlugin::shutdownPlugin() {
  // unregister all publishers here
}

void MyPlugin::saveSettings(qt_gui_cpp::Settings &plugin_settings,
                            qt_gui_cpp::Settings &instance_settings) const {
  // instance_settings.setValue(k, v)
}

void MyPlugin::restoreSettings(const qt_gui_cpp::Settings &plugin_settings,
                               const qt_gui_cpp::Settings &instance_settings) {
  // v = instance_settings.value(k)
}

void MyPlugin::getLeftJoyValue() {
  getJoyValue(joy_stick_left_, ui_.joyPosLabel);
}

void MyPlugin::getRightJoyValue() {
  getJoyValue(joy_stick_right_, ui_.joyPosLabel_3);
}

void MyPlugin::getJoyValue(JoyStick *joy, QLabel *joyPosLabel) {
  QString text = "(" + QString::number(joy->x_display_) + "," +
                 QString::number(joy->y_display_) + ")";
  joyPosLabel->setText(text);
}

void MyPlugin::topicNameUpdated() {
  ros::NodeHandle nh;
  if (ui_.topicLineEdit->text().toStdString() == "/" ||
      ui_.topicLineEdit->text().toStdString().c_str()[0] != '/') {
    ui_.topicLineEdit->setText(topic_name_);
    QMessageBox::warning(nullptr, "warn", "Please provide a useful topic name");
  }
  dbus_pub_ = nh.advertise<rm_msgs::DbusData>(topic_name_.toStdString(), 1);
  topic_name_ = ui_.topicLineEdit->text();
}

void MyPlugin::updatePublisher() {
  if (slip_button_->getState()) {
    dbus_pub_data_.ch_l_x = joy_stick_left_->x_display_;
    dbus_pub_data_.ch_l_y = joy_stick_left_->y_display_;
    dbus_pub_data_.ch_r_x = joy_stick_right_->x_display_;
    dbus_pub_data_.ch_r_y = joy_stick_right_->y_display_;
    dbus_pub_data_.stamp = ros::Time::now();
  }
}

void MyPlugin::startIntervalTimer() {
  pub_timer_->start(1000 / pub_rate_);
  pub_timer_->setSingleShot(false);
}

void MyPlugin::slipButtonChanged() {
  if (slip_button_->getState()) {
    if (ui_.topicLineEdit->text().toStdString() == "/" ||
        ui_.topicLineEdit->text().toStdString().c_str()[0] != '/') {
      QMessageBox::warning(
          nullptr, "dbus blocked",
          "Please provide a useful topic name and restart dbus");
      pub_timer_->stop();
      return;
    }
    startIntervalTimer();
  } else {
    pub_timer_->stop();
  }
}

void MyPlugin::publishRateSpinBoxChanged() {
  pub_rate_ = ui_.rateSpinBox->value();
  if (slip_button_->getState() && pub_timer_->isActive()) {
    startIntervalTimer();
  }
}

void MyPlugin::updateROSPublishState() {
  if (slip_button_->getState()) {
    dbus_pub_.publish(dbus_pub_data_);
  }
}

} // namespace rqt_virtual_dbus
PLUGINLIB_EXPORT_CLASS(rqt_virtual_dbus::MyPlugin, rqt_gui_cpp::Plugin)