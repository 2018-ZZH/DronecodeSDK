#include "global_include.h"
#include "control_impl.h"
#include "dronelink_impl.h"
#include "telemetry.h"
#include <unistd.h>

namespace dronelink {

ControlImpl::ControlImpl() :
    _in_air_state_known(false),
    _in_air(false)
{
}

ControlImpl::~ControlImpl()
{

}

void ControlImpl::init()
{
    using namespace std::placeholders; // for `_1`

    _parent->register_mavlink_message_handler(MAVLINK_MSG_ID_EXTENDED_SYS_STATE,
        std::bind(&ControlImpl::process_extended_sys_state, this, _1), (void *)this);
}

void ControlImpl::deinit()
{
    _parent->unregister_all_mavlink_message_handlers((void *)this);
}

Result ControlImpl::arm() const
{
    if (!is_arm_allowed()) {
        return Result::COMMAND_DENIED;
    }

    return _parent->send_command_with_ack(MAV_CMD_COMPONENT_ARM_DISARM,
                                          {1.0f, NAN, NAN, NAN, NAN, NAN, NAN});
}

Result ControlImpl::disarm() const
{
    if (!is_disarm_allowed()) {
        return Result::COMMAND_DENIED;
    }

    return _parent->send_command_with_ack(MAV_CMD_COMPONENT_ARM_DISARM,
                                          {0.0f, NAN, NAN, NAN, NAN, NAN, NAN});
}

Result ControlImpl::kill() const
{
    return _parent->send_command_with_ack(MAV_CMD_COMPONENT_ARM_DISARM,
                                          {0.0f, NAN, NAN, NAN, NAN, NAN, NAN});
}

Result ControlImpl::takeoff() const
{
    return _parent->send_command_with_ack(MAV_CMD_NAV_TAKEOFF,
                                          {NAN, NAN, NAN, NAN, NAN, NAN, NAN});
}

Result ControlImpl::land() const
{
    return _parent->send_command_with_ack(MAV_CMD_NAV_LAND,
                                          {NAN, NAN, NAN, NAN, NAN, NAN, NAN});
}

Result ControlImpl::return_to_land() const
{
    uint8_t mode = MAV_MODE_AUTO_ARMED | VEHICLE_MODE_FLAG_CUSTOM_MODE_ENABLED;
    uint8_t custom_mode = PX4_CUSTOM_MAIN_MODE_AUTO;
    uint8_t custom_sub_mode = PX4_CUSTOM_SUB_MODE_AUTO_RTL;

    return _parent->send_command_with_ack(MAV_CMD_DO_SET_MODE,
                                          {float(mode),
                                           float(custom_mode),
                                           float(custom_sub_mode),
                                           NAN, NAN, NAN, NAN});
}

void ControlImpl::arm_async(result_callback_t callback)
{
    if (!is_arm_allowed()) {
        report_result(callback, Result::COMMAND_DENIED);
        return;
    }

    _parent->send_command_with_ack_async(MAV_CMD_COMPONENT_ARM_DISARM,
                                         {1.0f, NAN, NAN, NAN, NAN, NAN, NAN},
                                         {callback, nullptr});
}

void ControlImpl::disarm_async(result_callback_t callback)
{
    if (!is_disarm_allowed()) {
        report_result(callback, Result::COMMAND_DENIED);
        return;
    }

    _parent->send_command_with_ack_async(MAV_CMD_COMPONENT_ARM_DISARM,
                                         {0.0f, NAN, NAN, NAN, NAN, NAN, NAN},
                                         {callback, nullptr});
}

void ControlImpl::kill_async(result_callback_t callback)
{
    _parent->send_command_with_ack_async(MAV_CMD_COMPONENT_ARM_DISARM,
                                         {0.0f, NAN, NAN, NAN, NAN, NAN, NAN},
                                         {callback, nullptr});
}

void ControlImpl::takeoff_async(result_callback_t callback)
{
    _parent->send_command_with_ack_async(MAV_CMD_NAV_TAKEOFF,
                                         {NAN, NAN, NAN, NAN, NAN, NAN, NAN},
                                         {callback, nullptr});
}

void ControlImpl::land_async(result_callback_t callback)
{
    _parent->send_command_with_ack_async(MAV_CMD_NAV_LAND,
                                         {NAN, NAN, NAN, NAN, NAN, NAN, NAN},
                                         {callback, nullptr});
}

void ControlImpl::return_to_land_async(result_callback_t callback)
{

    uint8_t mode = MAV_MODE_AUTO_ARMED | VEHICLE_MODE_FLAG_CUSTOM_MODE_ENABLED;
    uint8_t custom_mode = PX4_CUSTOM_MAIN_MODE_AUTO;
    uint8_t custom_sub_mode = PX4_CUSTOM_SUB_MODE_AUTO_RTL;

    _parent->send_command_with_ack_async(MAV_CMD_DO_SET_MODE,
                                         {float(mode),
                                          float(custom_mode),
                                          float(custom_sub_mode),
                                          NAN, NAN, NAN, NAN},
                                         {callback, nullptr});
}

bool ControlImpl::is_arm_allowed() const
{
    if (!_in_air_state_known) {
        return false;
    }

    if (_in_air) {
        return false;
    }

    return true;
}

bool ControlImpl::is_disarm_allowed() const
{
    if (!_in_air_state_known) {
        Debug() << "in air state unknown";
        return false;
    }

    if (_in_air) {
        Debug() << "still in air";
        return false;
    }

    return true;
}

void ControlImpl::process_extended_sys_state(const mavlink_message_t &message)
{
    mavlink_extended_sys_state_t extended_sys_state;
    mavlink_msg_extended_sys_state_decode(&message, &extended_sys_state);
    if (extended_sys_state.landed_state == MAV_LANDED_STATE_IN_AIR) {
        _in_air = true;
    } else if (extended_sys_state.landed_state == MAV_LANDED_STATE_ON_GROUND) {
        _in_air = false;
    }
    _in_air_state_known = true;
}

void ControlImpl::report_result(result_callback_t callback, Result result)
{
    if (callback == nullptr) {
        return;
    }

    callback(result, nullptr);
}

} // namespace dronelink