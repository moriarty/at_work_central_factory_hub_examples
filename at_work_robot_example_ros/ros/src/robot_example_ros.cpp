#include <at_work_robot_example_ros/robot_example_ros.h>


RobotExampleROS::RobotExampleROS(const ros::NodeHandle &nh):
    nh_(nh), seq_(0), 
    peer_public_(NULL),
    peer_team_(NULL)
{
    readParameters();

    //Publisher
    attention_message_pub_ = nh_.advertise<at_work_robot_example_ros::AttentionMessage> (
                            "attention_message", 10);

    benchmark_state_pub_ = nh_.advertise<at_work_robot_example_ros::BenchmarkState> (
                            "benchmark_state", 10);

    drill_machine_status_pub_ = nh_.advertise<at_work_robot_example_ros::DrillingMachineStatus> (
                            "drill_machine_status", 10);

    conveyor_belt_status_pub_ = nh_.advertise<at_work_robot_example_ros::TriggeredConveyorBeltStatus> (
                        "conveyor_belt_status", 10);

    inventory_pub_ = nh_.advertise<at_work_robot_example_ros::Inventory> ("inventory", 10);

    order_info_pub_ = nh_.advertise<at_work_robot_example_ros::OrderInfo> ("order_info", 10);

    initializeRobot();
}

RobotExampleROS::~RobotExampleROS()
{
    // Delete all global objects allocated by libprotobuf
    google::protobuf::ShutdownProtobufLibrary();
}

void RobotExampleROS::readParameters()
{
    ros::param::param<bool>("~remote_refbox", remote_refbox_, false);
    ros::param::param<std::string>("~host_name", host_name_, "localhost");

    //Paramters to use when ref box is running on remote machine.
    ros::param::param<int>("~public_port", public_port_, 4444);
    ros::param::param<int>("~team_port", team_port_, 4452);

    //Paramters to use when ref box is running on same machine as client.
    ros::param::param<int>("~refbox_send_port", public_recv_port_ , 4444);
    ros::param::param<int>("~refbox_recv_port", public_send_port_, 4445);
    ros::param::param<int>("~team_send_port", team_send_port_, 4453);
    ros::param::param<int>("~team_recv_port", team_recv_port_, 4452);

    ros::param::param<std::string>("~robot_name", robot_name_, "spqr");
    ros::param::param<std::string>("~team_name", team_name_, "SPQR");

    ROS_INFO("Hostname: %s", host_name_.c_str());
    ROS_INFO("Team Port: %i", team_port_);
    ROS_INFO("Public Port: %i", public_port_);
    ROS_INFO("Name: %s", robot_name_.c_str());
    ROS_INFO("Team Name: %s", team_name_.c_str());
}

void RobotExampleROS::initializeRobot()
{
    //create public peer
    if (remote_refbox_) {
        //ref box is running on remote machine.
        peer_public_.reset(new ProtobufBroadcastPeer(host_name_, 
                                                    public_port_));
    } else {
        //ref box is running on same machine as client.
        peer_public_.reset(new ProtobufBroadcastPeer(host_name_, 
                                                    public_send_port_, 
                                                    public_recv_port_));    
    }
    

    //create internal message handler
    MessageRegister &message_register = peer_public_->message_register();
    //added messagetype to the handler
    message_register.add_message_type<AttentionMessage>();
    message_register.add_message_type<BeaconSignal>();
    message_register.add_message_type<BenchmarkState>();
    message_register.add_message_type<BenchmarkFeedback>();
    message_register.add_message_type<Inventory>();
    message_register.add_message_type<OrderInfo>();
    message_register.add_message_type<RobotInfo>();
    message_register.add_message_type<VersionInfo>();
    message_register.add_message_type<DrillingMachineStatus>();
    message_register.add_message_type<TriggeredConveyorBeltStatus>();
    message_register.add_message_type<DrillingMachineCommand>();
    message_register.add_message_type<TriggeredConveyorBeltCommand>();

    //create team peer and linked to internal message handler
    if (remote_refbox_) {
        //ref box is running on remote machine.
        peer_team_.reset(new ProtobufBroadcastPeer(host_name_,
                                                    team_port_, 
                                                    &message_register));
    } else {
        //ref box is running on same machine as client.
        peer_team_.reset(new ProtobufBroadcastPeer(host_name_,
                                                    team_send_port_, 
                                                    team_recv_port_, 
                                                    &message_register));
    }

    //bind the peers to the callback funktions
    peer_public_->signal_received().connect(boost::bind(&RobotExampleROS::handleMessage, this, _1, _2, _3, _4));
    peer_public_->signal_send_error().connect(boost::bind( &RobotExampleROS::handleSendError, this, _1));
    peer_public_->signal_recv_error().connect(boost::bind(&RobotExampleROS::handleReceiveError, this, _1, _2));

    peer_team_->signal_received().connect(boost::bind(&RobotExampleROS::handleMessage, this, _1, _2, _3, _4));
    peer_team_->signal_send_error().connect(boost::bind( &RobotExampleROS::handleSendError, this, _1));
    peer_team_->signal_recv_error().connect(boost::bind(&RobotExampleROS::handleReceiveError, this, _1, _2));
}

void RobotExampleROS::sendBeacon()
{
    //generate the timestamp
    timespec start;
    clock_gettime(CLOCK_REALTIME, &start);
    int32_t sec = start.tv_sec;
    int32_t nsec = start.tv_nsec;

    //create the message
    std::shared_ptr<BeaconSignal> signal(new BeaconSignal());

    //seperate the time segment of the message
    Time *time = signal->mutable_time(); 
    //write the timestamp into the message
    time->set_sec(sec);
    time->set_nsec(nsec);

    //write the name and sequence counter into the message
    signal->set_peer_name(robot_name_);
    signal->set_team_name(team_name_);
    //increase the sequence number
    signal->set_seq(++seq_);
    //send over team peer       
    peer_team_->send(signal);
}

void RobotExampleROS::handleSendError(std::string msg)
{
    ROS_WARN("Send error: %s\n", msg.c_str());
}

void RobotExampleROS::handleReceiveError(boost::asio::ip::udp::endpoint &endpoint,
                        std::string msg)
{
    ROS_WARN("Recv error: %s\n", msg.c_str());
}

void RobotExampleROS::handleMessage(boost::asio::ip::udp::endpoint &sender,
                    uint16_t component_id, uint16_t msg_type,
                    std::shared_ptr<google::protobuf::Message> msg)
{
    std::shared_ptr<AttentionMessage> attention_msg_ptr;

    std::shared_ptr<BenchmarkState> benchmark_state_ptr;

    std::shared_ptr<DrillingMachineStatus> drill_machine_status_ptr;

    std::shared_ptr<TriggeredConveyorBeltStatus> conveyor_belt_status_ptr;

    std::shared_ptr<Inventory> inventory_pub_ptr;

    std::shared_ptr<OrderInfo> order_info_ptr;
             
    if ((attention_msg_ptr = std::dynamic_pointer_cast<AttentionMessage>(msg))) {

        at_work_robot_example_ros::AttentionMessage attention_msg;

        attention_msg.message.data      = attention_msg_ptr->message();
        attention_msg.time_to_show.data = attention_msg_ptr->time_to_show();
        attention_msg.team.data         = attention_msg_ptr->team();

        attention_message_pub_.publish(attention_msg);

    } else if ((benchmark_state_ptr = std::dynamic_pointer_cast<BenchmarkState>(msg))) {

        at_work_robot_example_ros::BenchmarkState benchmark_state_msg;

        benchmark_state_msg.benchmark_time.data.sec =
                                        benchmark_state_ptr->benchmark_time().sec();
        benchmark_state_msg.benchmark_time.data.nsec =
                                        benchmark_state_ptr->benchmark_time().nsec();
        benchmark_state_msg.state.data =
                                        benchmark_state_ptr->state();
        benchmark_state_msg.phase.data =
                                        benchmark_state_ptr->phase();
        benchmark_state_msg.scenario.benchmark_type.data =
                                        benchmark_state_ptr->scenario().type();
        benchmark_state_msg.scenario.benchmark_type_id.data =
                                        benchmark_state_ptr->scenario().type_id();
        benchmark_state_msg.scenario.benchmark_description.data =
                                        benchmark_state_ptr->scenario().description();

        benchmark_state_msg.known_teams.resize(benchmark_state_ptr->known_teams().size());

        for(int i=0; i < benchmark_state_ptr->known_teams().size(); i++) {
            benchmark_state_msg.known_teams[i].data =
                                        benchmark_state_ptr->known_teams(i);
        }

        benchmark_state_msg.connected_teams.resize(benchmark_state_ptr->connected_teams().size());

        for(int i=0; i < benchmark_state_ptr->connected_teams().size(); i++) {
            benchmark_state_msg.connected_teams[i].data =
                                        benchmark_state_ptr->connected_teams(i);
        }

        attention_message_pub_.publish(benchmark_state_msg);

    } else if ((drill_machine_status_ptr = std::dynamic_pointer_cast<DrillingMachineStatus>(msg))) {

        at_work_robot_example_ros::DrillingMachineStatus drill_machine_msg;

        drill_machine_msg.state.data = drill_machine_status_ptr->state();

        attention_message_pub_.publish(drill_machine_msg);

    } else if ((conveyor_belt_status_ptr = std::dynamic_pointer_cast<TriggeredConveyorBeltStatus>(msg))) {

        at_work_robot_example_ros::TriggeredConveyorBeltStatus conveyor_belt_status_msg;

        conveyor_belt_status_msg.state.data = conveyor_belt_status_ptr->state();

        conveyor_belt_status_msg.cycle.data = conveyor_belt_status_ptr->cycle();

        attention_message_pub_.publish(conveyor_belt_status_msg);

    } else if ((inventory_pub_ptr = std::dynamic_pointer_cast<Inventory>(msg))) {

        at_work_robot_example_ros::Inventory inventory_msg;

        inventory_msg.items.resize(inventory_pub_ptr->items().size());

        for(int i=0; i < inventory_pub_ptr->items().size(); i++) {

            inventory_msg.items[i].object.type.data =
                                        inventory_pub_ptr->items(i).object().type();

            inventory_msg.items[i].object.type_id.data =
                                        inventory_pub_ptr->items(i).object().type_id();

            inventory_msg.items[i].object.instance_id.data =
                                        inventory_pub_ptr->items(i).object().instance_id();

            inventory_msg.items[i].object.description.data =
                                        inventory_pub_ptr->items(i).object().description();

            inventory_msg.items[i].quantity.data =
                                        inventory_pub_ptr->items(i).quantity();

            inventory_msg.items[i].container.type.data =
                                        inventory_pub_ptr->items(i).container().type();

            inventory_msg.items[i].container.type_id.data =
                                        inventory_pub_ptr->items(i).container().type_id();

            inventory_msg.items[i].container.instance_id.data =
                                        inventory_pub_ptr->items(i).container().instance_id();

            inventory_msg.items[i].container.description.data =
                                        inventory_pub_ptr->items(i).container().description();

            inventory_msg.items[i].location.type.data =
                                        inventory_pub_ptr->items(i).location().type();

            inventory_msg.items[i].location.instance_id.data =
                                        inventory_pub_ptr->items(i).location().instance_id();

            inventory_msg.items[i].location.description.data =
                                        inventory_pub_ptr->items(i).location().description();
        }

        attention_message_pub_.publish(inventory_msg);

    }  else if ((order_info_ptr = std::dynamic_pointer_cast<OrderInfo>(msg))) {

        at_work_robot_example_ros::OrderInfo order_info_msg;

        order_info_msg.orders.resize(order_info_ptr->orders().size());

        for(int i=0; i < inventory_pub_ptr->items().size(); i++) {

            order_info_msg.orders[i].id.data =
                                        order_info_ptr->orders(i).id();
            order_info_msg.orders[i].status.data =
                                        order_info_ptr->orders(i).status();

            order_info_msg.orders[i].object.type.data =
                                        order_info_ptr->orders(i).object().type();

            order_info_msg.orders[i].object.type_id.data =
                                        order_info_ptr->orders(i).object().type_id();

            order_info_msg.orders[i].object.instance_id.data =
                                        order_info_ptr->orders(i).object().instance_id();

            order_info_msg.orders[i].object.description.data =
                                        order_info_ptr->orders(i).object().description();

            order_info_msg.orders[i].container.type.data =
                                        order_info_ptr->orders(i).container().type();

            order_info_msg.orders[i].container.type_id.data =
                                        order_info_ptr->orders(i).container().type_id();

            order_info_msg.orders[i].container.instance_id.data =
                                        order_info_ptr->orders(i).container().instance_id();

            order_info_msg.orders[i].container.description.data =
                                        order_info_ptr->orders(i).container().description();

            order_info_msg.orders[i].quantity_delivered.data =
                                        order_info_ptr->orders(i).quantity_delivered();

            order_info_msg.orders[i].quantity_requested.data =
                                        order_info_ptr->orders(i).quantity_requested();

            order_info_msg.orders[i].destination.type.data =
                                        order_info_ptr->orders(i).destination().type();

            order_info_msg.orders[i].destination.instance_id.data =
                                        order_info_ptr->orders(i).destination().instance_id();

            order_info_msg.orders[i].destination.description.data =
                                        order_info_ptr->orders(i).destination().description();

            order_info_msg.orders[i].source.type.data =
                                        order_info_ptr->orders(i).source().type();

            order_info_msg.orders[i].source.instance_id.data =
                                        order_info_ptr->orders(i).source().instance_id();

            order_info_msg.orders[i].source.description.data =
                                        order_info_ptr->orders(i).source().description();

            order_info_msg.orders[i].processing_team.data =
                                        order_info_ptr->orders(i).processing_team();
        }

        attention_message_pub_.publish(order_info_msg);

    }
}
