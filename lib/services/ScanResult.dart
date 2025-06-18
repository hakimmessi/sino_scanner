
import 'PassportData.dart';

class ScanResult {
  final bool success;
  final String? error;
  final PassportData? passportData;
  final Map<String, dynamic>? rawScanData;
  final bool imagesSaved;
  final List<String>? savedImagePaths;

  ScanResult._({
    required this.success,
    this.error,
    this.passportData,
    this.rawScanData,
    this.imagesSaved = false,
    this.savedImagePaths,
  });

  factory ScanResult.success({
    PassportData? passportData,
    Map<String, dynamic>? rawScanData,
    bool imagesSaved = false,
    List<String>? savedImagePaths,
  }) {
    return ScanResult._(
      success: true,
      passportData: passportData,
      rawScanData: rawScanData,
      imagesSaved: imagesSaved,
      savedImagePaths: savedImagePaths,
    );
  }

  factory ScanResult.error(String error) {
    return ScanResult._(
      success: false,
      error: error,
    );
  }
}