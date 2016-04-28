#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
#include <ros/ros.h>
#include <protobuf_comm/peer.h>

#include <raw_pb_msgs/AttentionMessage.pb.h>
#include <raw_pb_msgs/BeaconSignal.pb.h>
#include <raw_pb_msgs/BenchmarkState.pb.h>
#include <raw_pb_msgs/BenchmarkFeedback.pb.h>
#include <raw_pb_msgs/ConveyorBelt.pb.h>
#include <raw_pb_msgs/Inventory.pb.h>
#include <raw_pb_msgs/Order.pb.h>
#include <raw_pb_msgs/RobotInfo.pb.h>
#include <raw_pb_msgs/Time.pb.h>
#include <raw_pb_msgs/VersionInfo.pb.h>
#include <raw_pb_msgs/LoggingStatus.pb.h>
#include <raw_pb_msgs/RobotStatusReport.pb.h>
#include <raw_pb_msgs/TaskSpecification.pb.h>

//publisher
#include <at_work_robot_example_ros/AttentionMessage.h>
#include <at_work_robot_example_ros/BenchmarkState.h>
//#include <at_work_robot_example_ros/TriggeredConveyorBeltStatus.h>
#include <at_work_robot_example_ros/Inventory.h>
#include <at_work_robot_example_ros/OrderInfo.h>
#include <std_msgs/String>

// subscribers
#include <at_work_robot_example_ros/BenchmarkFeedback.h>
//#include <at_work_robot_example_ros/TriggeredConveyorBeltCommand.h>
//#include <at_work_robot_example_ros/DrillingMachineCommand.h>
#include <at_work_robot_example_ros/LoggingStatus.h>
#include <at_work_robot_example_ros/Transaction.h>
#include <at_work_robot_example_ros/RobotStatusReport.h>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <unistd.h>

#include <sstream>

using namespace protobuf_comm;
using namespace raw_pb_msgs;

class RobotExampleROS
{
    public:
        /**
         * Ctor.
         */
        RobotExampleROS(const ros::NodeHandle &nh);

        /**
         * Dtor.
         */
        ~RobotExampleROS();

        void readParameters();

        void initializeRobot();

        /**
         * Beacon Signal
         *
         * This function sends the beacon signal
         * The function is called from the main loop
         */
        void sendBeacon();

    private:
        /**
         * Copy Ctor.
         */
        RobotExampleROS(const RobotExampleROS &other);

        /**
         * Assignment operator
         */
        RobotExampleROS &operator=(const RobotExampleROS &other);

        /**
         * Handler for send errors.
         *
         * This handler is called for send errors.
         * Print the message that should be send as ROS Warning
         *
         */
        void handleSendError(std::string msg);

        /**
         * Handler for receive errors.
         *
         * This handler is called for receive errors.
         * Print the message that should be received as a ROS Warning
         *
         */
        void handleReceiveError(boost::asio::ip::udp::endpoint &endpoint,
                                std::string msg);

        /**
         * Handler for receive messages .
         *
         * This handler is called for receiving messages.
         * Main function in this node.
         *
         */
        void handleMessage(boost::asio::ip::udp::endpoint &sender,
                            uint16_t component_id, uint16_t msg_type,
                            std::shared_ptr<google::protobuf::Message> msg);


        //void DrillingMachineCommandCB(at_work_robot_example_ros::DrillingMachineCommand msg);


        //void TriggeredConveyorBeltCommandCB(at_work_robot_example_ros::TriggeredConveyorBeltCommand msg);


        //void BenchmarkFeedbackCB(at_work_robot_example_ros::BenchmarkFeedback msg);

        void LoggingStatusCB(at_work_robot_example_ros::LoggingStatus msg);

        void InventoryTransactionCB(at_work_robot_example_ros::Transaction msg);

        void RobotStatusReportCB(at_work_robot_example_ros::RobotStatusReport msg);

    private:
        /**
         * ROS node handle.
         */
        ros::NodeHandle nh_;

        /**
         * Stores sequence of the bacon messages.
         */
        unsigned long seq_;

        /**
         * The interface to the Protobuf Broadcast peer to communicate with Refbox.
         */
        std::shared_ptr<ProtobufBroadcastPeer> peer_public_;

        /**
         * The interface to the Protobuf Broadcast peer to communicate with Team.
         */
        std::shared_ptr<ProtobufBroadcastPeer> peer_team_;

        /**
         * Stores robot name.
         */
        std::string robot_name_;

        /**
         * Stores team name.
         */
        std::string team_name_;

        /**
         * Publishers
         */
        ros::Publisher attention_message_pub_;

        ros::Publisher benchmark_state_pub_;

        //ros::Publisher drill_machine_status_pub_;
        ros::Publisher task_spec_pub_;
        
        ros::Publisher inventory_pub_;

        ros::Publisher order_info_pub_;

        //ros::Publisher conveyor_belt_status_pub_;

        /**
         * Subscribers
         */
        //ros::Subscriber drillling_machine_command_sub_;

        //ros::Subscriber conveyor_belt_command_sub_;

        ros::Subscriber benchmark_feedback_sub_;

        ros::Subscriber logging_status_sub_;

        ros::Subscriber transaction_sub_;

        ros::Subscriber robot_status_sub_;

        /**
         * Parameter to check if refbox is running on local or another machine.
         */
        bool remote_refbox_;

        /**
         * Parameter to store host name(IP address).
         */
        std::string host_name_;

        /**
         * Stores port number on which refbox is receiving or sending messages
         * when the refbox is running on another machine. 
         */
        int public_port_;

        /**
         * Stores port number on which robot/team is receiving or sending messages. 
         * when the refbox is running on another machine.
         */
        int team_port_;

        /**
         * Stores port number on which refbox is sending messages. 
         * when the refbox is running on local machine.
         */
        int public_send_port_;

        /**
         * Stores port number on which refbox is receiving messages. 
         * when the refbox is running on local machine.
         */
        int public_recv_port_;
 
        /**
         * Stores port number on which the team is sending messages. 
         * when the refbox is running on local machine.
         */
        int team_send_port_;
 
        /**
         * Stores port number on which team is receiving messages. 
         * when the refbox is running on local machine.
         */
        int team_recv_port_;
};
