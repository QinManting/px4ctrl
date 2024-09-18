gnome-terminal -t "QGroundControl" -x bash -c "./QGroundControl.AppImage ;exec bash;"
sleep 2s
gnome-terminal -t "DSP" -x bash -c "cd DSP-MAP/map_ws/ ;
source devel/setup.bash ;
roslaunch dynamic_occpuancy_map mapping.launch ;exec bash;"
sleep 1s
gnome-terminal -t "gazebo" -x bash -c "cd px4_sim_ws/src/uav-px4ctrl-1-sim_uav_px4ctrl/sim/PX4-Autopilot/ ;
source px4origin.bash ;
roslaunch px4 fast_test.launch ;exec bash;"
sleep 1s
gnome-terminal -t "px4" -x bash -c "cd px4_sim_ws/src/uav-px4ctrl-1-sim_uav_px4ctrl/sim/sim_uav_px4ctrl/ ;
source devel/setup.bash ;
roslaunch px4ctrl run_node.launch ;exec bash;"
sleep 1s
gnome-terminal -t "rqt_reconfigure" -x bash -c "rosrun rqt_reconfigure rqt_reconfigure ;exec bash;"
sleep 1s
gnome-terminal -t "pseudo_path" -x bash -c "cd mpc_ws/; 
source devel/setup.bash; roslaunch pseudo_path pseudo_path.launch ;exec bash;"
sleep 1s
gnome-terminal -t "controller" -x bash -c "cd mpc_ws/; 
source devel/setup.bash; roslaunch controller mpc_controller_sim.launch ;exec bash;"
sleep 1s

