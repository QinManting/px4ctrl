#include <ros/ros.h>
#include "PX4Ctrlfsm.h"
#include <signal.h>

void mySigintHandler(int sig)
{
    ROS_INFO("[PX4Ctrl] exit...");
    ros::shutdown();
}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "px4ctrl");
    ros::NodeHandle nh;
    ros::NodeHandle nh_("~");

    signal(SIGINT, mySigintHandler);
    ros::Duration(1.0).sleep();
    //初始化参数类  载入的是ctrl_param_fpv.yaml
    Parameter_t param;
    param.config_from_ros_handle(nh_);

    // Controller controller(param);
    //初始化线性控制器
    LinearControl controller(param);
    //初始化状态机 默认手动模式
    PX4CtrlFSM fsm(param, controller);
    //订阅状态
    ros::Subscriber state_sub =
        nh.subscribe<mavros_msgs::State>("mavros/state",
                                         10,
                                         boost::bind(&State_Data_t::feed, &fsm.state_data, _1));
    //订阅扩展状态
    ros::Subscriber extended_state_sub =
        nh.subscribe<mavros_msgs::ExtendedState>("mavros/extended_state",
                                                 10,
                                                 boost::bind(&ExtendedState_Data_t::feed, &fsm.extended_state_data, _1));
    //订阅vins传回的里程计数据
    ros::Subscriber odom_sub =
        nh.subscribe<nav_msgs::Odometry>("odom",
                                         100,
                                         boost::bind(&Odom_Data_t::feed, &fsm.odom_data, _1),
                                         ros::VoidConstPtr(),
                                         ros::TransportHints().tcpNoDelay());
    //订阅position_cmd
    ros::Subscriber cmd_sub =
        nh.subscribe<quadrotor_msgs::PositionCommand>("cmd",
                                                      100,
                                                      boost::bind(&Command_Data_t::feed, &fsm.cmd_data, _1),
                                                      ros::VoidConstPtr(),
                                                      ros::TransportHints().tcpNoDelay());
    //订阅imu数u
    ros::Subscriber imu_sub =
        nh.subscribe<sensor_msgs::Imu>("mavros/imu/data", // Note: do NOT change it to /mavros/imu/data_raw !!!
                                       100,
                                       boost::bind(&Imu_Data_t::feed, &fsm.imu_data, _1),
                                       ros::VoidConstPtr(),
                                       ros::TransportHints().tcpNoDelay());
    //订阅遥控器数据
    ros::Subscriber rc_sub;
    //仿真时不订阅
    if (!param.takeoff_land.no_RC) // mavros will still publish wrong rc messages although no RC is connected
    {
        rc_sub = nh.subscribe<mavros_msgs::RCIn>("mavros/rc/in",
                                                 10,
                                                 boost::bind(&RC_Data_t::feed, &fsm.rc_data, _1));
    }
    //订阅电池电量
    // ros::Subscriber bat_sub =
    //     nh.subscribe<sensor_msgs::BatteryState>("/mavros/battery",
    //                                             100,
    //                                             boost::bind(&Battery_Data_t::feed, &fsm.bat_data, _1),
    //                                             ros::VoidConstPtr(),
    //                                             ros::TransportHints().tcpNoDelay());
    //订阅自动起飞命令
    ros::Subscriber takeoff_land_sub =
        nh.subscribe<quadrotor_msgs::TakeoffLand>("takeoff_land",
                                                  100,
                                                  boost::bind(&Takeoff_Land_Data_t::feed, &fsm.takeoff_land_data, _1),
                                                  ros::VoidConstPtr(),
                                                  ros::TransportHints().tcpNoDelay());

    //发布姿态和位置
    fsm.ctrl_FCU_pub = nh.advertise<mavros_msgs::AttitudeTarget>("mavros/setpoint_raw/attitude", 10);
    fsm.ctrl_mpc_pub = nh.advertise<mavros_msgs::PositionTarget>("/mavros/setpoint_raw/local", 10);
    
    fsm.traj_start_trigger_pub = nh.advertise<std_msgs::Bool>("/traj_start_trigger", 10);

    fsm.debug_pub = nh.advertise<quadrotor_msgs::Px4ctrlDebug>("/debugPx4ctrl", 10); // debug
    //三个客户端
    fsm.set_FCU_mode_srv = nh.serviceClient<mavros_msgs::SetMode>("mavros/set_mode");
    fsm.arming_client_srv = nh.serviceClient<mavros_msgs::CommandBool>("mavros/cmd/arming");
    fsm.reboot_FCU_srv = nh.serviceClient<mavros_msgs::CommandLong>("mavros/cmd/command");

    ros::Duration(0.5).sleep();

    dynamic_reconfigure::Server<px4ctrl::fake_rcConfig> server;
    dynamic_reconfigure::Server<px4ctrl::fake_rcConfig>::CallbackType f;

    
    if (param.takeoff_land.no_RC)
    {
        //仿真时
        f = boost::bind(&Dynamic_Data_t::feed, &fsm.dy_data ,_1); //绑定回调函数
        server.setCallback(f); //为服务器设置回调函数， 节点程序运行时会调用一次回调函数来输出当前的参数配置情况
        ROS_WARN("PX4CTRL] Remote controller disabled, be careful!");
    }
    else
    {
        //实机飞行
        ROS_INFO("PX4CTRL] Waiting for RC");
        while (ros::ok())//等待遥控器连接
        {
            ros::spinOnce();
            if (fsm.rc_is_received(ros::Time::now()))
            {
                ROS_INFO("[PX4CTRL] RC received.");
                break;
            }
            ros::Duration(0.1).sleep();
        }
    }

    int trials = 0;//等待px4连接
    while (ros::ok() && !fsm.state_data.current_state.connected)
    {
        ros::spinOnce();
        ros::Duration(1.0).sleep();
        if (trials++ > 5)
            ROS_ERROR("Unable to connnect to PX4!!!");
    }

    ros::Rate r(param.ctrl_freq_max);
    while (ros::ok())
    {
        ROS_INFO_ONCE("PX4CTRL] Is OK!");
        r.sleep();
        ros::spinOnce();
        fsm.process(); // We DO NOT rely on feedback as trigger, since there is no significant performance difference through our test.
    }

    return 0;
}