#pragma once
#include <unordered_map>
#include <string>
namespace CPPRP {
	static const std::unordered_map<std::string, std::string> class_extensions =
	{
		  {"Engine.Actor", "Core.Object"}
		, {"Engine.Info","Engine.Actor"}
		, {"Engine.ReplicationInfo","Engine.Info"}
		, {"Engine.GameReplicationInfo","Engine.ReplicationInfo"}
		, {"Engine.Pawn","Engine.Actor"}
		, {"Engine.PlayerReplicationInfo","Engine.ReplicationInfo"}
		, {"Engine.TeamInfo","Engine.ReplicationInfo"}
		, {"Engine.WorldInfo","Engine.Info"}
		, {"Engine.ADynamicSMActor","Engine.Actor"}
		, {"Engine.KActor","Engine.ADynamicSMActor"}
		, {"ProjectX.GRI_X","Engine.GameReplicationInfo"}
		, {"ProjectX.NetModeReplicator_X","Engine.ReplicationInfo"}
		, {"ProjectX.Pawn_X","Engine.Pawn"}
		, {"ProjectX.PRI_X","Engine.PlayerReplicationInfo"}
		, {"TAGame.PRI_TA","ProjectX.PRI_X"}
		, {"TAGame.RBActor_TA","ProjectX.Pawn_X"}
		, {"TAGame.CarComponent_TA","Engine.ReplicationInfo"}
		, {"TAGame.CarComponent_Jump_TA","TAGame.CarComponent_TA"}
		, {"TAGame.CarComponent_DoubleJump_TA","TAGame.CarComponent_TA"}
		, {"TAGame.CarComponent_Boost_TA","TAGame.CarComponent_TA"}
		, {"TAGame.CarComponent_Dodge_TA","TAGame.CarComponent_TA"}
		, {"TAGame.CarComponent_FlipCar_TA","TAGame.CarComponent_TA"}
		, {"TAGame.Ball_TA","TAGame.RBActor_TA"}
		, {"TAGame.Team_TA","Engine.TeamInfo"}
		, {"TAGame.Team_Soccar_TA","TAGame.Team_TA"}
		, {"TAGame.BreakOutActor_Platform_TA","Engine.Actor"}
		, {"TAGame.SpecialPickup_TA","TAGame.CarComponent_TA"}
		, {"TAGame.SpecialPickup_Targeted_TA","TAGame.SpecialPickup_TA"}
		, {"TAGame.SpecialPickup_Tornado_TA","TAGame.SpecialPickup_TA"}
		, {"TAGame.SpecialPickup_HauntedBallBeam_TA","TAGame.SpecialPickup_TA"}
		, {"TAGame.SpecialPickup_BallVelcro_TA","TAGame.SpecialPickup_TA"}
		, {"TAGame.SpecialPickup_Rugby_TA","TAGame.SpecialPickup_TA"}
		, {"TAGame.SpecialPickup_BallFreeze_TA","TAGame.SpecialPickup_Targeted_TA"}
		, {"TAGame.SpecialPickup_Spring_TA","TAGame.SpecialPickup_Targeted_TA"}
		, {"TAGame.SpecialPickup_BallCarSpring_TA","TAGame.SpecialPickup_Spring_TA"}
		, {"TAGame.SpecialPickup_BallGravity_TA","TAGame.SpecialPickup_TA"}
		, {"TAGame.SpecialPickup_GrapplingHook_TA","TAGame.SpecialPickup_Targeted_TA"}
		, {"TAGame.SpecialPickup_BallLasso_TA","TAGame.SpecialPickup_GrapplingHook_TA"}
		, {"TAGame.SpecialPickup_BoostOverride_TA","TAGame.SpecialPickup_Targeted_TA"}
		, {"TAGame.SpecialPickup_Batarang_TA","TAGame.SpecialPickup_BallLasso_TA"}
		, {"TAGame.SpecialPickup_HitForce_TA","TAGame.SpecialPickup_TA"}
		, {"TAGame.SpecialPickup_Swapper_TA","TAGame.SpecialPickup_Targeted_TA"}
		, {"TAGame.CrowdManager_TA","Engine.ReplicationInfo"}
		, {"TAGame.CrowdActor_TA","Engine.ReplicationInfo"}
		, {"TAGame.InMapScoreboard_TA","Engine.Actor"}
		, {"TAGame.Vehicle_TA","TAGame.RBActor_TA"}
		, {"TAGame.Car_TA","TAGame.Vehicle_TA"}
		, {"TAGame.Car_Season_TA","TAGame.Car_TA"}
		, {"TAGame.CameraSettingsActor_TA","Engine.ReplicationInfo"}
		, {"TAGame.GRI_TA","ProjectX.GRI_X"}
		, {"TAGame.Ball_Breakout_TA","TAGame.Ball_TA"}
		, {"TAGame.Ball_God_TA","TAGame.Ball_TA"}
		, {"TAGame.VehiclePickup_TA","Engine.ReplicationInfo"}
		, {"TAGame.VehiclePickup_Boost_TA","TAGame.VehiclePickup_TA"}
		, {"TAGame.Ball_Haunted_TA","TAGame.Ball_TA"}
		, {"TAGame.GameEvent_TA","Engine.ReplicationInfo"}
		, {"TAGame.GameEvent_Team_TA","TAGame.GameEvent_TA"}
		, {"TAGame.GameEvent_Soccar_TA","TAGame.GameEvent_Team_TA"}
		, {"TAGame.GameEvent_Breakout_TA","TAGame.GameEvent_Soccar_TA"}
		, {"TAGame.GameEvent_GodBall_TA","TAGame.GameEvent_Soccar_TA"}
		, {"TAGame.GameEvent_Season_TA","TAGame.GameEvent_Soccar_TA"}
		, {"TAGame.GameEvent_SoccarPrivate_TA","TAGame.GameEvent_Soccar_TA"}
		, {"TAGame.GameEvent_SoccarSplitscreen_TA","TAGame.GameEvent_SoccarPrivate_TA"}
		, {"TAGame.GameEvent_Tutorial_TA","TAGame.GameEvent_Soccar_TA"}
		, {"TAGame.GameEvent_GameEditor_TA","TAGame.GameEvent_Soccar_TA"}
		, {"TAGame.GameEditor_Pawn_TA","ProjectX.Pawn_X"}
		, {"TAGame.GameEvent_TrainingEditor_TA","TAGame.GameEvent_GameEditor_TA"}
		, {"TAGame.HauntedBallTrapTrigger_TA","Engine.Actor"}
		, {"TAGame.Cannon_TA","Engine.Actor"}
	};
};

