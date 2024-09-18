gnome-terminal -t "rviz" -x bash -c "cd ego-v2+map_generator/src/EGO-Planner-v2-with-DSP-map-accumulated/swarm-playground/main_ws/ ;
source devel/setup.bash ;
roslaunch ego_planner rviz.launch ;exec bash;"
sleep 1s
gnome-terminal -t "rviz" -x bash -c "cd ego-v2+map_generator/src/EGO-Planner-v2-with-DSP-map-accumulated/swarm-playground/main_ws/ ;
source devel/setup.bash ;
roslaunch ego_planner single_drone_waypoints_STS.launch ;exec bash;"
sleep 1s
