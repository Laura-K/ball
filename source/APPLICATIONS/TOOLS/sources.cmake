### the directory name
SET(DIRECTORY source/APPLICATIONS/TOOLS)

SET(EXECUTABLES_LIST
	AddMissingAtoms
	AntitargetRescorer
	AutoModel
	BindingDBCleaner
	BondOrderAssigner
	CalculateBindingFreeEnergy
	CalculateEnergy
	CalculateSolvationFreeEnergy
	CombiLibGenerator
	ConstraintsFinder
	Converter
	CrystalGenerator
	DBExporter
	DBImporter
	DockPoseClustering
	DockResultMerger
	EvenSplit
	ExtractClustersFromWardTree
	ExtractProteinChains
	ExtractProteinSequence
	FeatureSelector
	FingerprintSimilarityClustering
	FingerprintSimilaritySearch
	GridBuilder
	IMGDock
	InputPartitioner
	InputReader
	InteractionConstraintDefiner
	LigandFileSplitter
	LigCheck
	ModelCreator
	MolCombine
	#MolecularFileConverter
	MolFilter
	MolPredictor
	PartialChargesCopy
	PDBCutter
	PDBDownload
	PDBRMSDCalculator
	PeptideBuilder
	PocketDetector
	PoseIndices2PDB
	Predictor
	PropertyModifier
	PropertyPlotter
	ProteinCheck
	RemoveWater
	ResidueChecker
	RMSDCalculator
	ScoreAnalyzer
	SideChainGridBuilder
	SimilarityAnalyzer
	SimpleRescorer
	SLICK
	SpatialConstraintDefiner
	Split2ConnectedComponents
	TaGRes
	TaGRes-train
	TrajectoryFile2PDBSplitter
	Trajectory2RigidTransformation
	Validator
	VendorFinder
	WaterFinder
	# add new programs here
)

IF(BALL_HAS_OPENBABEL OR BALL_HAS_OEPNEYE)
	LIST(APPEND EXECUTABLES_LIST
		Ligand3DGenerator
		MolDepict
		ProteinProtonator
	)
ENDIF()

SET(TOOLS_EXECUTABLES ${TOOLS_EXECUTABLES} ${EXECUTABLES_LIST})

### add filenames to Visual Studio solution
SET(TOOLS_SOURCES)
FOREACH(i ${EXECUTABLES_LIST})
	LIST(APPEND TOOLS_SOURCES "${i}")
ENDFOREACH(i)
SOURCE_GROUP("" FILES ${TOOLS_SOURCES})
