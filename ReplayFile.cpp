#include "ReplayFile.h"
#include <fstream>
#include "networkdata.h"
#include "rapidjson/filewritestream.h"
#include <rapidjson/writer.h>

ReplayFile::ReplayFile(std::filesystem::path path_) : path(path_)
{
	
}


ReplayFile::~ReplayFile()
{
}

bool ReplayFile::Load()
{
	if (!std::filesystem::exists(path))
		return false;
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	/*file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		file.open(path, std::ios::binary);
	}
	catch (std::ifstream::failure e) {
		std::cerr << "Exception opening file: " << std::strerror(errno) << "\n";
	}*/
	
	std::streamsize size = file.tellg();
	data.resize((size_t)size);
	file.seekg(0, std::ios::beg);
	
	if (file.bad())
		return false;
	return (bool)file.read(data.data(), size);
}

void ReplayFile::DeserializeHeader()
{
	const uint32_t dataSize = data.size() / 4;
	replayFile = std::make_shared<ReplayFileData>();
	fullReplayBitReader = CPPBitReader<uint32_t>((const uint32_t*)data.data(), dataSize, replayFile); //They're read as bytes, since we're retrieving them in memory as uint32_t, divide size by 4 (bytes)
	//replayFile->header.crc = fullReplayBitReader.read<uint32_t>();
	replayFile->header = {
		fullReplayBitReader.read<uint32_t>(),	//Size
		fullReplayBitReader.read<uint32_t>(),	//CRC
		fullReplayBitReader.read<uint32_t>(),	//engineVersion
		fullReplayBitReader.read<uint32_t>()	//licenseeVersion
	};

	if (replayFile->header.engineVersion >= 868 && replayFile->header.licenseeVersion >= 18)
	{
		replayFile->header.netVersion = fullReplayBitReader.read<uint32_t>();
	}
	replayFile->replayType = fullReplayBitReader.read<std::string>(); //Not sure what this is


	while (true) {
		auto baseProperty = std::make_shared<Property>();
		const bool moreToParse = ParseProperty(baseProperty);
		if (!moreToParse)
		{
			break;
		}
		replayFile->properties[baseProperty->property_name] = baseProperty;
	}
	//replayFile->properties = baseProperty;
	replayFile->body_size = fullReplayBitReader.read<uint32_t>();
	replayFile->crc2 = fullReplayBitReader.read<uint32_t>();

	const uint32_t levelCount = fullReplayBitReader.read<uint32_t>();
	for (uint32_t i = 0; i < levelCount; ++i)
	{
		replayFile->levels.push_back(fullReplayBitReader.read<std::string>());
	}

	const uint32_t keyframeCount = fullReplayBitReader.read<uint32_t>();
	for (uint32_t i = 0; i < keyframeCount; ++i)
	{
		replayFile->keyframes.push_back(
			{
				fullReplayBitReader.read<float>(),	//Time
				fullReplayBitReader.read<uint32_t>(),	//Frame
				fullReplayBitReader.read<uint32_t>()	//File position
			});
	}

	const uint32_t netstreamCount = static_cast<uint32_t>(fullReplayBitReader.read<int32_t>());
	replayFile->netstream_data = data.data() + fullReplayBitReader.GetBytePosition(); //We know this is always aligned, so valid
	uint32_t test = netstreamCount * 8;
	fullReplayBitReader.skip(test);
	replayFile->netstream_size = netstreamCount;

	const int32_t debugStringSize = fullReplayBitReader.read<int32_t>();
	for (int32_t i = 0; i < debugStringSize; ++i)
	{
		uint32_t frame = fullReplayBitReader.read<uint32_t>();
		std::string key = fullReplayBitReader.read<std::string>();
		std::string value = fullReplayBitReader.read<std::string>();
		///printf("%s = %s", key.c_str(), value.c_str());
	}
	//fullReplayBitReader.skip(4*8); //debug_log apparently

	const uint32_t replayTickCount = fullReplayBitReader.read<uint32_t>();
	for (uint32_t i = 0; i < replayTickCount; ++i)
	{
		replayFile->replayticks.push_back(
			{
				fullReplayBitReader.read<std::string>(),	//Type
				fullReplayBitReader.read<uint32_t>()		//Frame
			});
	}


	const uint32_t replicatedPackagesCount = fullReplayBitReader.read<uint32_t>();
	for (uint32_t i = 0; i < replicatedPackagesCount; ++i)
	{
		replayFile->replicated_packages.push_back(
			{
				fullReplayBitReader.read<std::string>()
			});
	}

	const uint32_t objectsCount = fullReplayBitReader.read<uint32_t>();
	for (uint32_t i = 0; i < objectsCount; ++i)
	{
		replayFile->objects.push_back(
			{
				fullReplayBitReader.read<std::string>()
			});
	}

	const uint32_t namesCount = fullReplayBitReader.read<uint32_t>();
	for (uint32_t i = 0; i < namesCount; ++i)
	{
		replayFile->names.push_back(
			{
				fullReplayBitReader.read<std::string>()
			});
	}

	const uint32_t classIndexCount = fullReplayBitReader.read<uint32_t>();
	for (uint32_t i = 0; i < classIndexCount; ++i)
	{
		replayFile->class_indices.push_back(
			{
				fullReplayBitReader.read<std::string>(),	//Class_name
				fullReplayBitReader.read<uint32_t>()		//Index
			});
	}

	const uint32_t classNetsCount = fullReplayBitReader.read<uint32_t>();
	replayFile->classnets.resize(classNetsCount);
	for (uint32_t i = 0; i < classNetsCount; ++i)
	{
		ClassNet cn = {
			fullReplayBitReader.read<int32_t>(),		//Index
			fullReplayBitReader.read<int32_t>(),		//Parent
			NULL,							//Parent class, not known yet
			fullReplayBitReader.read<int32_t>(),		//Id
			fullReplayBitReader.read<int32_t>(),		//Prop_indexes_size
			std::vector<PropIndexId>(),		//Empty propindexid array
			0,								//Max_prop_id
			std::vector<uint16_t>()			//Property_id_cache
		};

		const uint32_t newSize = cn.prop_indexes_size;
		cn.prop_indexes.resize(newSize);
		for (uint32_t j = 0; j < newSize; ++j)
		{
			cn.prop_indexes[j] = (
				PropIndexId	{
					fullReplayBitReader.read<int32_t>(),	//Prop_index
					fullReplayBitReader.read<int32_t>()	//Prop_id
				});
		}
		std::shared_ptr<ClassNet> classNet = std::make_shared<ClassNet>(cn);
		replayFile->classnets[i] = (classNet);

		//Set parent class if exists
		int ow = 0;
		for (int32_t k = (int32_t)i - 1; k >= 0; --k)
		{
			if (replayFile->classnets[i]->parent == replayFile->classnets[k]->id)
			{
				replayFile->classnets[i]->parent_class = replayFile->classnets[k];
				break;
			}
		}
	}
	if (replayFile->header.netVersion >= 10)
	{
		fullReplayBitReader.read<int32_t>();
	}
	networkParser.RegisterParsers(replayFile);
}

void ReplayFile::MergeDuplicates()
{
	GetClassnetByName("");
	std::unordered_map<std::string, int> counts;
	for (auto it : classnetMap)
	{
		if (it.second)
		{
			counts[replayFile->objects[it.second->index]]++;
		}
		else int p = 5;
	}
	int o = 5;
}

const std::unordered_map<std::string, std::string> class_extensions = 
{
	{"Engine.Actor", "Core.Object"}
  , {"Engine.GameReplicationInfo", "Engine.ReplicationInfo"}
  , {"Engine.Info", "Engine.Actor"}
  , {"Engine.Pawn", "Engine.Actor"}
  , {"Engine.PlayerReplicationInfo", "Engine.ReplicationInfo"}
  , {"Engine.ReplicationInfo", "Engine.Info"}
  , {"Engine.TeamInfo", "Engine.ReplicationInfo"}
  , {"ProjectX.GRI_X", "Engine.GameReplicationInfo"}
  , {"ProjectX.Pawn_X", "Engine.Pawn"}
  , {"ProjectX.PRI_X", "Engine.PlayerReplicationInfo"}
  , {"TAGame.Ball_TA", "TAGame.RBActor_TA"}
  , {"TAGame.CameraSettingsActor_TA", "Engine.ReplicationInfo"}
  , {"TAGame.Car_Season_TA", "TAGame.PRI_TA"}
  , {"TAGame.Car_TA", "TAGame.Vehicle_TA"}
  , {"TAGame.CarComponent_Boost_TA", "TAGame.CarComponent_TA"}
  , {"TAGame.CarComponent_Dodge_TA", "TAGame.CarComponent_TA"}
  , {"TAGame.CarComponent_DoubleJump_TA", "TAGame.CarComponent_TA"}
  , {"TAGame.CarComponent_FlipCar_TA", "TAGame.CarComponent_TA"}
  , {"TAGame.CarComponent_Jump_TA", "TAGame.CarComponent_TA"}
  , {"TAGame.CarComponent_TA", "Engine.ReplicationInfo"}
  , {"TAGame.CrowdActor_TA", "Engine.ReplicationInfo"}
  , {"TAGame.CrowdManager_TA", "Engine.ReplicationInfo"}
  , {"TAGame.GameEvent_Season_TA", "TAGame.GameEvent_Soccar_TA"}
  , {"TAGame.GameEvent_Soccar_TA", "TAGame.GameEvent_Team_TA"}
  , {"TAGame.GameEvent_SoccarPrivate_TA", "TAGame.GameEvent_Soccar_TA"}
  , {"TAGame.GameEvent_SoccarSplitscreen_TA", "TAGame.GameEvent_SoccarPrivate_TA"}
  , {"TAGame.GameEvent_TA", "Engine.ReplicationInfo"}
  , {"TAGame.GameEvent_Team_TA", "TAGame.GameEvent_TA"}
  , {"TAGame.GRI_TA", "ProjectX.GRI_X"}
  , {"TAGame.InMapScoreboard_TA", "Engine.Actor"}
  , {"TAGame.PRI_TA", "ProjectX.PRI_X"}
  , {"TAGame.RBActor_TA", "ProjectX.Pawn_X"}
  , {"TAGame.SpecialPickup_BallCarSpring_TA", "TAGame.SpecialPickup_Spring_TA"}
  , {"TAGame.SpecialPickup_BallFreeze_TA", "TAGame.SpecialPickup_Targeted_TA"}
  , {"TAGame.SpecialPickup_BallGravity_TA", "TAGame.SpecialPickup_TA"}
  , {"TAGame.SpecialPickup_BallLasso_TA", "TAGame.SpecialPickup_GrapplingHook_TA"}
  , {"TAGame.SpecialPickup_BallVelcro_TA", "TAGame.SpecialPickup_TA"}
  , {"TAGame.SpecialPickup_Batarang_TA", "TAGame.SpecialPickup_BallLasso_TA"}
  , {"TAGame.SpecialPickup_BoostOverride_TA", "TAGame.SpecialPickup_Targeted_TA"}
  , {"TAGame.SpecialPickup_GrapplingHook_TA", "TAGame.SpecialPickup_Targeted_TA"}
  , {"TAGame.SpecialPickup_HitForce_TA", "TAGame.SpecialPickup_TA"}
  , {"TAGame.SpecialPickup_Spring_TA", "TAGame.SpecialPickup_Targeted_TA"}
  , {"TAGame.SpecialPickup_Swapper_TA", "TAGame.SpecialPickup_Targeted_TA"}
  , {"TAGame.SpecialPickup_TA", "TAGame.CarComponent_TA"}
  , {"TAGame.SpecialPickup_Targeted_TA", "TAGame.SpecialPickup_TA"}
  , {"TAGame.SpecialPickup_Tornado_TA", "TAGame.SpecialPickup_TA"}
  , {"TAGame.Team_Soccar_TA", "TAGame.Team_TA"}
  , {"TAGame.Team_TA", "Engine.TeamInfo"}
  , {"TAGame.Vehicle_TA", "TAGame.RBActor_TA"}
  , {"TAGame.VehiclePickup_Boost_TA", "TAGame.VehiclePickup_TA"}
  , {"TAGame.VehiclePickup_TA", "Engine.ReplicationInfo"}
  , {"TAGame.SpecialPickup_HauntedBallBeam_TA", "TAGame.SpecialPickup_TA"}
  , {"TAGame.CarComponent_TA", "Engine.Actor"}
  , {"Engine.Info", "Engine.Actor"}
  , {"Engine.Pawn", "Engine.Actor"}
};

void ReplayFile::FixParents()
{
	this->MergeDuplicates();
	for (auto kv : class_extensions)
	{
		
		std::shared_ptr<ClassNet> childClass = GetClassnetByName(kv.first);
		std::shared_ptr<ClassNet> parentClass = GetClassnetByName(kv.second);
		if (parentClass != nullptr && childClass != nullptr && (childClass->parent_class == nullptr || (childClass->parent_class->id != parentClass->id)))
		{
			childClass->parent_class = parentClass;
		}
	}
	for (auto cn : replayFile->classnets)
	{
		uint32_t i = 0;
		uint32_t result = GetPropertyIndexById(cn, i);
		while (result != 0)
		{
			cn->property_id_cache.push_back(result);
			result = GetPropertyIndexById(cn, ++i);
		}
	}

}

uint32_t val = 0;
void ReplayFile::Parse(std::string fileName, const uint32_t startPos, int32_t endPos)
{
	if (endPos < 0)
	{
		endPos = replayFile->netstream_size / 4 * 8;
	}
	//Divide by 4 since netstream_data is bytes, but we read uint32_ts
	CPPBitReader<uint32_t> networkReader((uint32_t*)(replayFile->netstream_data), ((uint32_t)endPos), replayFile);

	++val;
	FILE* fp = fopen(("./json/" + fileName + ".json").c_str(), "wb");

	try {
		char writeBuffer[65536 * 5];
		rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

		rapidjson::Writer<rapidjson::FileWriteStream> writer(os);

		std::unordered_map<uint32_t, std::string> test;

		networkReader.skip(startPos);

		writer.StartObject();
		writer.String("frames");
		const int32_t maxChannels = GetProperty<int32_t>("MaxChannels");
		writer.StartArray();
		while (networkReader.canRead())
		{
			writer.StartObject();
			Frame f;
			f.time = networkReader.read<float>();
			f.delta = networkReader.read<float>();

			writer.String("time");
			writer.Double(f.time);
			writer.String("delta");
			writer.Double(f.delta);
			int k = 5;

			writer.String("actors");
			writer.StartArray();
			//While there are actors in buffer (this frame)
			while (networkReader.read<bool>())
			{
				writer.StartObject();
				const uint32_t actorId = networkReader.readBitsMax<uint32_t>(maxChannels);
				ActorState& actorState = actorStates[actorId];
				writer.String("actorid");
				writer.Uint(actorId);

				writer.String("status");
				if (networkReader.read<bool>())
				{

					//Is new state
					if (networkReader.read<bool>())
					{
						writer.String("created");
						if (replayFile->header.engineVersion > 868 || (replayFile->header.engineVersion == 868 && replayFile->header.licenseeVersion >= 14))
						{

							actorState.name_id = networkReader.read<uint32_t>();
							writer.String("nameid");
							writer.Uint(actorState.name_id);
						}
						const bool unknownBool = networkReader.read<bool>();
						const uint32_t typeId = networkReader.read<uint32_t>();
						writer.String("typeid");
						writer.Uint(typeId);
						//const uint32_t bit_pos = networkReader.GetAbsoluteBitPosition();

						const std::string typeName = replayFile->objects.at(typeId);
						writer.String("typename");
						writer.String(typeName.c_str(), typeName.size());
						actorState.classNet = GetClassnetByNameWithLookup(typeName);

						if (actorState.classNet == nullptr)
							throw 22;

						const uint32_t classId = actorState.classNet->index;
						const std::string className = replayFile->objects.at(classId);

						writer.String("classname");
						writer.String(className.c_str(), className.size());

						if (HasInitialPosition(className))
						{
							actorState.position = static_cast<Vector3>(networkReader.read<Vector3I>());
							writer.String("initialposition");
							Serialize(writer, actorState.position);
						}
						if (HasRotation(className))
						{
							actorState.rotation = networkReader.read<Rotator>();
							writer.String("initialrotation");
							Serialize(writer, actorState.rotation);
						}
					}
					else //Is existing state
					{
						writer.String("updated");
						writer.String("updates");
						writer.StartArray();
						//While there's data for this state to be updated
						while (networkReader.read<bool>())
						{
							writer.StartObject();
							const uint16_t maxPropId = GetMaxPropertyId(actorState.classNet);
							const uint32_t propertyId = networkReader.readBitsMax<uint32_t>(maxPropId + 1);
							const uint32_t propertyIndex = actorState.classNet->property_id_cache[propertyId];
							//printf("Calling parser for %s (%i, %i)\n", replayFile->objects[propertyIndex].c_str(), propertyIndex, actorId);
							writer.String("class");
							writer.String(replayFile->objects[propertyIndex].c_str(), replayFile->objects[propertyIndex].size());

							writer.String("data");
							//printf("Calling parse for %s", )
							networkParser.Parse(propertyIndex, networkReader, writer);
							writer.EndObject();
						}
						writer.EndArray();
					}
				}
				else
				{
					writer.String("deleted");
				}
				writer.EndObject();
			}
			writer.EndArray();
			writer.EndObject();
		}

		writer.EndArray();
		writer.EndObject();
	}
	catch (...)
	{
		fclose(fp);
		throw 5;
	}
	
	fclose(fp);
	//printf("Parsed\n");
}

const bool ReplayFile::HasInitialPosition(const std::string & name) const
{
	return !(name.compare("TAGame.CrowdActor_TA") == 0
		|| name.compare("TAGame.CrowdManager_TA") == 0
		|| name.compare("TAGame.VehiclePickup_Boost_TA") == 0
		|| name.compare("TAGame.InMapScoreboard_TA") == 0
		|| name.compare("TAGame.BreakOutActor_Platform_TA") == 0
		|| name.compare("Engine.WorldInfo") == 0
		|| name.compare("TAGame.HauntedBallTrapTrigger_TA") == 0);
}

const bool ReplayFile::HasRotation(const std::string & name) const
{
	return name.compare("TAGame.Ball_TA") == 0
		|| name.compare("TAGame.Car_TA") == 0
		|| name.compare("TAGame.Car_Season_TA") == 0
		|| name.compare("TAGame.Ball_Breakout_TA") == 0
		|| name.compare("TAGame.Ball_Haunted_TA") == 0;
}

const std::pair<const uint32_t, const KeyFrame> ReplayFile::GetNearestKeyframe(uint32_t frame) const
{
	if (replayFile->keyframes.size() == 0)
	{
		return std::make_pair<const uint32_t, KeyFrame>(0, { 0.f, 0,0 });
	}
	
	const size_t size = replayFile->keyframes.size();
	size_t currentKeyframeIndex = 0;
	for (currentKeyframeIndex; currentKeyframeIndex < size; ++currentKeyframeIndex)
	{
		if (replayFile->keyframes.at(currentKeyframeIndex).frame > frame)
		{
			break;
		}
	}
	
	const KeyFrame nearestKeyFrame = replayFile->keyframes.at(currentKeyframeIndex);
	const uint32_t frameNumber = nearestKeyFrame.frame;
	return std::make_pair(frameNumber, nearestKeyFrame);
}

const bool ReplayFile::ParseProperty(const std::shared_ptr<Property>& currentProperty)
{
	currentProperty->property_name = fullReplayBitReader.read<std::string>();
	if (currentProperty->property_name.compare("None") == 0) //We're done parsing this prop
	{
		return false;
	}
	currentProperty->property_type = fullReplayBitReader.read<std::string>();
	const uint32_t propertySize = fullReplayBitReader.read<uint32_t>();
	const uint32_t idk = fullReplayBitReader.read<uint32_t>();

	//Not sure why I'm doing these micro optimizations here, kinda hurts readability and its only like a nanosecond
	switch (currentProperty->property_type[0])
	{
	case 'N':
	{
		if (currentProperty->property_type[1] == 'o') //Type is "None"
		{
			return false;
		}
		else //Type is "Name"
		{
			currentProperty->value = fullReplayBitReader.read<std::string>();
		}
	}
		break;
	case 'I': //IntProperty
	{
		currentProperty->value = fullReplayBitReader.read<int32_t>();
	}
	break;
	case 'S': //StringProperty
	{
		currentProperty->value = fullReplayBitReader.read<std::string>();
	}
	break;
	case 'B':
	{
		if (currentProperty->property_type[1] == 'y') //Type is "ByteProperty"
		{
			currentProperty->value = EnumProperty
			{ 
				fullReplayBitReader.read<std::string>(),	//Type
				fullReplayBitReader.read<std::string>()	//Value
			};
		}
		else //Type is "BoolProperty", but unlike network data, is stored as entire byte
		{
			currentProperty->value = fullReplayBitReader.read<uint8_t>();
		}
	}
	break;
	case 'Q': //QWordProperty
	{
		currentProperty->value = fullReplayBitReader.read<uint64_t>();
	}
	break;
	case 'F': //FloatProperty
	{
		currentProperty->value = fullReplayBitReader.read<float>();
	}
	break;
	case 'A': //ArrayProperty
	{
		const int32_t count = fullReplayBitReader.read<int32_t>();
		std::vector<std::unordered_map<std::string, std::shared_ptr<Property>>> properties;
		properties.resize(count);
		
		for (int32_t i = 0; i < count; ++i) 
		{
			std::unordered_map<std::string, std::shared_ptr<Property>> props;
			while (true) 
			{
				auto baseProperty = std::make_shared<Property>();
				const bool moreToParse = ParseProperty(baseProperty);
				if (!moreToParse)
				{
					break;
				}
				props[currentProperty->property_name] = baseProperty;
			}
			properties[i] = props;
		}

		/*for (int32_t i = 0; i < count; ++i)
		{
			std::shared_ptr<Property> prop = std::make_shared<Property>();
			ParseProperty(prop);
			properties[i] = prop;
		}*/
		currentProperty->value = properties;
	}
	break;
	default: //Die
		//assert(1 == 2);
		break;
	}
	
	return true;
}

const std::shared_ptr<ClassNet>& ReplayFile::GetClassnetByName(const std::string& name)
{
	if (!classNetMapCached)
	{
		for (uint32_t i = 0; i < replayFile->classnets.size(); ++i)
		{
			const uint32_t index = replayFile->classnets.at(i)->index;
			const std::string objectName = replayFile->objects.at(index);
			if (objectName.compare(objectName) == 0)
			{
				classnetMap[objectName] = replayFile->classnets.at(i);
			}
		}

		classNetMapCached = true;
	}
	return classnetMap[name];
}

const std::shared_ptr<ClassNet>& ReplayFile::GetClassnetByNameWithLookup(const std::string & name)
{
	if (name.compare("Archetypes.Car.Car_Default") == 0) {
		return GetClassnetByName("TAGame.Car_TA");
	}
	if (name.compare("Archetypes.Ball.Ball_Default") == 0 || name.compare("Archetypes.Ball.Ball_Basketball") == 0 ||
		name.compare("Archetypes.Ball.Ball_BasketBall") == 0 || name.compare("Archetypes.Ball.Ball_BasketBall_Mutator") == 0 ||
		name.compare("Archetypes.Ball.Ball_Puck") == 0 || name.compare("Archetypes.Ball.CubeBall") == 0 ||
		name.compare("Archetypes.Ball.Ball_Beachball") == 0) {
		return GetClassnetByName("TAGame.Ball_TA");
	}
	if (name.compare("Archetypes.Ball.Ball_Breakout") == 0) {
		return GetClassnetByName("TAGame.Ball_Breakout_TA");
	}
	if (name.compare("Archetypes.CarComponents.CarComponent_Boost") == 0) {
		return GetClassnetByName("TAGame.CarComponent_Boost_TA");
	}
	if (name.compare("Archetypes.CarComponents.CarComponent_Dodge") == 0) {
		return GetClassnetByName("TAGame.CarComponent_Dodge_TA");
	}
	if (name.compare("Archetypes.CarComponents.CarComponent_DoubleJump") == 0) {
		return GetClassnetByName("TAGame.CarComponent_DoubleJump_TA");
	}
	if (name.compare("Archetypes.CarComponents.CarComponent_FlipCar") == 0) {
		return GetClassnetByName("TAGame.CarComponent_FlipCar_TA");
	}
	if (name.compare("Archetypes.CarComponents.CarComponent_Jump") == 0) {
		return GetClassnetByName("TAGame.CarComponent_Jump_TA");
	}
	if (name.compare("Archetypes.Teams.Team0") == 0 || name.compare("Archetypes.Teams.Team1") == 0) {
		return GetClassnetByName("TAGame.Team_Soccar_TA");
	}
	if (name.compare("TAGame.Default__PRI_TA") == 0) {
		return GetClassnetByName("TAGame.PRI_TA");
	}
	if (name.compare("Archetypes.GameEvent.GameEvent_Basketball") == 0 || name.compare("Archetypes.GameEvent.GameEvent_Hockey") == 0 ||
		name.compare("Archetypes.GameEvent.GameEvent_Soccar") == 0 || name.compare("Archetypes.GameEvent.GameEvent_Items") == 0 || name.compare("Archetypes.GameEvent.GameEvent_SoccarLan") == 0) {
		return GetClassnetByName("TAGame.GameEvent_Soccar_TA");
	}
	if (name.compare("Archetypes.GameEvent.GameEvent_SoccarPrivate") == 0 || name.compare("Archetypes.GameEvent.GameEvent_BasketballPrivate") == 0 || name.compare("Archetypes.GameEvent.GameEvent_HockeyPrivate") == 0) {
		return GetClassnetByName("TAGame.GameEvent_SoccarPrivate_TA");
	}
	if (name.compare("Archetypes.GameEvent.GameEvent_SoccarSplitscreen") == 0 || name.compare("Archetypes.GameEvent.GameEvent_BasketballSplitscreen") == 0 || name.compare("Archetypes.GameEvent.GameEvent_HockeySplitscreen") == 0) {
		return GetClassnetByName("TAGame.GameEvent_SoccarSplitscreen_TA");
	}
	if (name.compare("Archetypes.GameEvent.GameEvent_Season") == 0) {
		return GetClassnetByName("TAGame.GameEvent_Season_TA");
	}
	if (name.compare("Archetypes.GameEvent.GameEvent_Season:CarArchetype") == 0) {
		return GetClassnetByName("TAGame.Car_TA");
	}
	if (name.compare("Archetypes.GameEvent.GameEvent_Breakout") == 0) {
		return GetClassnetByName("TAGame.GameEvent_Breakout_TA");
	}
	if (name.compare("GameInfo_Basketball.GameInfo.GameInfo_Basketball:GameReplicationInfoArchetype") == 0 || name.compare("Gameinfo_Hockey.GameInfo.Gameinfo_Hockey:GameReplicationInfoArchetype") == 0
		|| name.compare("GameInfo_Season.GameInfo.GameInfo_Season:GameReplicationInfoArchetype") == 0 || name.compare("GameInfo_Soccar.GameInfo.GameInfo_Soccar:GameReplicationInfoArchetype") == 0
		|| name.compare("GameInfo_Items.GameInfo.GameInfo_Items:GameReplicationInfoArchetype") == 0 || name.compare("GameInfo_Breakout.GameInfo.GameInfo_Breakout:GameReplicationInfoArchetype") == 0) {
		return GetClassnetByName("TAGame.GRI_TA");
	}
	if (name.compare("TAGame.Default__CameraSettingsActor_TA") == 0) {
		return GetClassnetByName("TAGame.CameraSettingsActor_TA");
	}
	if (name.compare("Neotokyo_p.TheWorld:PersistentLevel.InMapScoreboard_TA_0") == 0 || name.compare("NeoTokyo_P.TheWorld:PersistentLevel.InMapScoreboard_TA_0") == 0 ||
		name.compare("NeoTokyo_P.TheWorld:PersistentLevel.InMapScoreboard_TA_1") == 0 || name.compare("NeoTokyo_Standard_P.TheWorld:PersistentLevel.InMapScoreboard_TA_1") == 0 || name.compare("NeoTokyo_Standard_P.TheWorld:PersistentLevel.InMapScoreboard_TA_0") == 0) {
		return GetClassnetByName("TAGame.InMapScoreboard_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_GravityWell") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_BallGravity_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_BallVelcro") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_BallVelcro_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_BallLasso") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_BallLasso_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_BallGrapplingHook") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_GrapplingHook_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_Swapper") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_Swapper_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_BallFreeze") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_BallFreeze_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_BoostOverride") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_BoostOverride_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_Tornado") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_Tornado_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_CarSpring") == 0 || name.compare("Archetypes.SpecialPickups.SpecialPickup_BallSpring") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_BallCarSpring_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_StrongHit") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_HitForce_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_Batarang") == 0) {
		return GetClassnetByName("TAGame.SpecialPickup_Batarang_TA");
	}
	if (name.compare("Neotokyo_p.TheWorld:PersistentLevel.InMapScoreboard_TA_1") == 0) {
		return GetClassnetByName("TAGame.InMapScoreboard_TA");
	}
	if (name.compare("Archetypes.Ball.Ball_Haunted") == 0)
	{
		return GetClassnetByName("TAGame.Ball_Haunted_TA");
	}
	if (name.compare("Haunted_TrainStation_P.TheWorld:PersistentLevel.HauntedBallTrapTrigger_TA_1") == 0 ||
		name.compare("Haunted_TrainStation_P.TheWorld:PersistentLevel.HauntedBallTrapTrigger_TA_0") == 0)
	{
		return GetClassnetByName("TAGame.HauntedBallTrapTrigger_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_HauntedBallBeam") == 0)
	{
		return GetClassnetByName("TAGame.SpecialPickup_HauntedBallBeam_TA");
	}
	if (name.compare("Archetypes.SpecialPickups.SpecialPickup_Rugby") == 0)
	{
		return GetClassnetByName("TAGame.SpecialPickup_Rugby_TA");
	}


	if (name.find("CrowdActor_TA") != std::string::npos)
	{
		return GetClassnetByName("TAGame.CrowdActor_TA");
	}
	else if (name.find("VehiclePickup_Boost_TA") != std::string::npos)
	{
		return GetClassnetByName("TAGame.VehiclePickup_Boost_TA");
	}
	else if (name.find("CrowdManager_TA") != std::string::npos)
	{
		return GetClassnetByName("TAGame.CrowdManager_TA");
	}
	else if (name.find("BreakOutActor_Platform_TA") != std::string::npos)
	{
		return GetClassnetByName("TAGame.BreakOutActor_Platform_TA");
	}
	else if (name.find("WorldInfo") != std::string::npos)
	{
		return GetClassnetByName("Engine.WorldInfo");
	}
	else if (name.find("Archetypes.Teams.TeamWhite") != std::string::npos)
	{
		return GetClassnetByName("Engine.TeamInfo");
	}
	return GetClassnetByName(name);
}

const uint16_t ReplayFile::GetPropertyIndexById(const std::shared_ptr<ClassNet>& cn, const int id) const
{
	for (int i = 0; i < cn->prop_indexes_size; i++)
	{
		if (cn->prop_indexes[i].prop_id == id)
		{
			return cn->prop_indexes[i].prop_index;
		}
	}
	if (cn->parent_class)
	{
		const std::shared_ptr<ClassNet>& parentNet = cn->parent_class;
		if (parentNet == NULL) //Is root?
		{
			return cn->index;
		}
		return this->GetPropertyIndexById(parentNet, id);
	}
	return 0;
}

const uint16_t ReplayFile::GetMaxPropertyId(const std::shared_ptr<ClassNet>& cn)
{
	if (cn == nullptr)
		throw 21;
	if (cn->max_prop_id == 0)
	{
		cn->max_prop_id = FindMaxPropertyId(cn, 0);
	}
	return cn->max_prop_id;
}

const uint16_t ReplayFile::FindMaxPropertyId(const std::shared_ptr<ClassNet>& cn, uint16_t maxProp) const
{
	if (cn == nullptr)
	{
		return maxProp;
	}

	for (int32_t i = 0; i < cn->prop_indexes_size; ++i)
	{
		if (cn->prop_indexes[i].prop_id > maxProp)
		{
			maxProp = cn->prop_indexes[i].prop_id;
		}
	}
	if (cn->parent_class)
	{
		return FindMaxPropertyId(cn->parent_class, maxProp);
	}
	return maxProp;
}
