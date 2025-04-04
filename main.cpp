#include "lemlib/api.hpp" // IWYU pragma: keep
#include "liblvgl/llemu.hpp"
#include "pros/abstract_motor.hpp"
#include "pros/adi.hpp"
#include "pros/distance.hpp"
#include "pros/llemu.hpp"
#include "pros/misc.h"
#include "pros/motors.hpp"
#include "pros/optical.hpp"
#include "pros/rotation.hpp"
#include "main.h"
#include <functional>
#include <vector>

pros::MotorGroup left_motors({1, -2, 3}, pros::MotorGearset::blue); // left motors use 600 RPM cartridges
pros::MotorGroup right_motors({17, -19, -20}, pros::MotorGearset::blue); // right motors use 600 RPM cartridges

// drivetrain settings
lemlib::Drivetrain drivetrain(
	&left_motors, // left motor group
	&right_motors, // right motor group
	12, // 12  inch track width
	lemlib::Omniwheel::NEW_325, // using new 3.25" omnis
	450, // drivetrain rpm is 360
	2 // horizontal drift is 2 (for now)
);

pros::Motor Intake (11, pros::v5::MotorGears::blue);
pros::Motor LB (15, pros::v5::MotorGears::green);

pros::Distance Distance(16);

pros::Rotation Rotation(4);

pros::Optical Optical(5);

pros::adi::Pneumatics Doinker('H', false);
pros::adi::Button IntakeLS('G');
pros::adi::Pneumatics Clamp('B', false);
pros::adi::DigitalIn Button ('A');

double kP=.5;
double error;
double LBvel;
double LBPos;
int Target = 40;
/////LAdyBrown Functions////
void LBSet(){
    while (LBPos >= Target+1 or LBPos <= Target-1) {
        error=Target-LBPos;
        LBvel=error*kP;
        LB.move(LBvel);
    }
}



////////Auton select set up//////////
int autonCount = 0;
std::vector<std::string> AutonList = {"Blue", "Red", "AWP"};

// create an imu on port 13
pros::Imu imu(13);



//Creates controller

// odometry settings
lemlib::OdomSensors sensors(nullptr, // vertical tracking wheel 1, set to null
	nullptr, // vertical tracking wheel 2, set to nullptr as we are using IMEs
	nullptr, // horizontal tracking wheel 1
	nullptr, // horizontal tracking wheel 2, set to nullptr as we don't have a second one
	&imu // inertial sensor
);

// lateral PID controller
lemlib::ControllerSettings lateral_controller(10, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              3, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in inches
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              20 // maximum acceleration (slew)
);

// angular PID controller
lemlib::ControllerSettings angular_controller(2, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              10, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in degrees
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in degrees
                                              500, // large error range timeout, in milliseconds
                                              0 // maximum acceleration (slew)
);


// create the chassis
lemlib::Chassis chassis(drivetrain, // drivetrain settings
	lateral_controller, // lateral PID settings
	angular_controller, // angular PID settings
	sensors // odometry sensors
);


/**
 * A callback function for LLEMU's center button.
 *
 * When this callback is fired, it will toggle line 2 of the LCD text between
 * "I was pressed!" and nothing.
 */
void on_center_button() {
	static bool pressed = false;
	pressed = !pressed;
	if (pressed) {
		pros::lcd::set_text(2, "I was pressed!");
	} else {
		pros::lcd::clear_line(2);
	}
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
	pros::lcd::initialize(); // initialize brain screen
    chassis.calibrate(); // calibrate sensors
    // print position to brain screen
    pros::Task screen_task([&]() {
        while (true) {
            // print robot location to the brain screen
            pros::lcd::print(0, "X: %f", chassis.getPose().x); // x
            pros::lcd::print(1, "Y: %f", chassis.getPose().y); // y
            pros::lcd::print(2, "Theta: %f", chassis.getPose().theta); // heading
            // delay to save resources
            pros::delay(20);
        }
    });
	pros::lcd::initialize();
	pros::lcd::set_text(1, "Hello PROS User!");

	pros::lcd::register_btn1_cb(on_center_button);


    pros::lcd::set_text(4,"Selected Auton:");

    while(true){
        if (Button.get_new_press()) {
            autonCount=(autonCount+1)%AutonList.size();
        }
        pros::lcd::set_text(5, AutonList[autonCount]);
    }


}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {
    pros::lcd::set_text(1, "working Auton");
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol() {
	pros::Controller master(pros::E_CONTROLLER_MASTER);
	pros::MotorGroup left_mg({1, -2, 3});    // Creates a motor group with forwards ports 1 & 3 and reversed port 2
	pros::MotorGroup right_mg({-4, 5, -6});  // Creates a motor group with forwards port 5 and reversed ports 4 & 6
    pros::lcd::set_text(1, "working driver control");

	while (true) {
		pros::lcd::print(0, "%d %d %d", (pros::lcd::read_buttons() & LCD_BTN_LEFT) >> 2,
		                 (pros::lcd::read_buttons() & LCD_BTN_CENTER) >> 1,
		                 (pros::lcd::read_buttons() & LCD_BTN_RIGHT) >> 0);  // Prints status of the emulated screen LCDs

		// Arcade control scheme
		// get left y and right x positions
        int leftY = master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
        int rightX = master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X);

        // move the robot
        chassis.arcade(leftY, rightX);
        
        LBPos=Rotation.get_position();

        //////Intake COntrol///////
        if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
            Intake.move(100);
        } else if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
            Intake.move(-100);
        } else {
            Intake.move(0);
        }


        /////LadyBrown Control////////
        if (master.get_digital(pros::E_CONTROLLER_DIGITAL_LEFT)) {
            LBSet();
        }

        if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L1)) {
            LB.move(100);
        } else if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L2)) {
            LB.move(-100);
        } else {
            LB.move(0);
        }

        ////Pneumatic control///
        if (master.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN)) {
            Clamp.toggle();
        }
        if (master.get_digital(pros::E_CONTROLLER_DIGITAL_Y)) {
            Doinker.toggle();
        }

        // delay to save resources
        pros::delay(25);
	}
}
