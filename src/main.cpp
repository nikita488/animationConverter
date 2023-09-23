#include "main.h"

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		fprintf(stderr, "Usage : %s <inputFile.igb> <outputFile.fbx|dae>\n", argv[0]);
		return 1;
	}

	igAlchemy alchemy;
	Sg::igArkRegisterForIGBFiles();
	ArkCore->getPluginHelper()->loadPlugin("libRaven", "Alchemy");

	igString inputFile = argv[1];
	igString outputFile = argv[2];

	if (!igIGBResource->load(inputFile))
	{
		fprintf(stderr, "Failed to load IGB file %s\n", inputFile);
		return -1;
	}

	igAnimationDatabaseRef animDB = igAnimationDatabase::dynamicCast(igIGBResource->getInfoByType(inputFile, "igAnimationDatabase"));

	igIGBResource->unload(inputFile);

	if (!animDB)
	{
		fprintf(stderr, "Failed to get igAnimationDatabase from IGB file %s\n", inputFile);
		return -2;
	}

	igInt skelCount = animDB->getSkeletonList()->getCount();
	igInt animCount = animDB->getAnimationList()->getCount();

	if (skelCount != 1 || animCount < 1)
	{
		fprintf(stderr, "igAnimationDatabase must contain exactly 1 skeleton and 1 or more animations, found %d skeletons and %d animations\n", skelCount, animCount);
		return -3;
	}
	
	Converter::ConverterManager manager;

	if (!manager.Initialize(outputFile))
	{
		fprintf(stderr, "Failed to initialize exporter for %s\n", outputFile);
		return -4;
	}

	Converter::AnimationConverter animConverter(manager.GetScene(), animDB->getSkeleton(0), animDB->getAnimationList());

	animDB = NULL;
	animConverter.Convert();

	if (!manager.Export())
	{
		fprintf(stderr, "Failed to export %s\n", outputFile);
		return -5;
	}

	return 0;
}