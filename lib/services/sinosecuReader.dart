import 'dart:io';

import 'package:flutter/services.dart';

import 'ImagePathHelper.dart';
import 'PassportData.dart';
import 'ScanResult.dart';

class SinosecuReader {
  static const MethodChannel _channel = MethodChannel('com.example.sino_scanner');

  static Future<int> initializeScanner({
    required String userId,
    required int nType,
    required String sdkDirectory,
  }) async {
    try {
      print('[Flutter] Calling initializeScanner with:');
      print('[Flutter]   UserID: $userId');
      print('[Flutter]   nType: $nType');
      print('[Flutter]   SDK Directory: $sdkDirectory');

      final int result = await _channel.invokeMethod('initializeScanner', {
        'userId': userId,
        'nType': nType,
        'sdkDirectory': sdkDirectory,
      });
      print('[Flutter] Scanner initialized. Native Result: $result');
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to initialize scanner: ${e.message}');
      print('[Flutter] PlatformException Details: ${e.details}');
      return -100;
    } catch (e) {
      print('[Flutter] Unknown error during initializeScanner: $e');
      return -200;
    }
  }

  // The SDK's DetectDocument returns an int.
  static Future<int> detectDocument() async {
    try {
      final int result = await _channel.invokeMethod('detectDocument');
      print('[Flutter] detectDocument call. Native Result: $result');
      // Interpret result based on SDK documentation:
      // -1: Core engine not initialized
      //  0: Cannot detect
      //  1: Document placed
      //  2: Document taken out
      //  3: Detected mobile phone barcode (AR/KR series)
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to call detectDocument: ${e.message}');
      return -100;
    } catch (e) {
      print('[Flutter] Unknown error during detectDocument: $e');
      return -200;
    }
  }

  // Wait for document detection with timeout
  static Future<int> waitForDocumentDetection(int timeoutSeconds) async {
    try {
      print('[Flutter] Waiting for document detection (timeout: ${timeoutSeconds}s)');
      final int result = await _channel.invokeMethod('waitForDocumentDetection', {
        'timeoutSeconds': timeoutSeconds,
      });
      print('[Flutter] waitForDocumentDetection result: $result');
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to wait for document: ${e.message}');
      return -100;
    } catch (e) {
      print('[Flutter] Unknown error during waitForDocumentDetection: $e');
      return -200;
    }
  }

  // The SDK's AutoProcessIDCard returns an int (main document type or error code)
  // and has an output parameter nCardType. Platform channels return a Map containing both values.
  static Future<Map<String, dynamic>> autoProcessDocument() async {
    try {
      // C++ wrapper  returns a Map: {"status": result, "cardType": nCardTypeValue}
      final Map<dynamic, dynamic>? result = await _channel.invokeMethod('autoProcessDocument');
      print('[Flutter] autoProcessDocument call. Native Result: $result');
      if (result != null) {
        return Map<String, dynamic>.from(result);
      }
      return {"error": "Null result from native", "status": -999, "cardType": -999};
    } on PlatformException catch (e) {
      print('[Flutter] Failed to call autoProcessDocument: ${e.message}');
      return {"error": e.message, "status": -100, "cardType": -100};
    } catch (e) {
      print('[Flutter] Unknown error during autoProcessDocument: $e');
      return {"error": e.toString(), "status": -200, "cardType": -200};
    }
  }

  // Get document fields (OCR or chip data)
  static Future<Map<String, dynamic>> getDocumentFields(int attribute) async {
    try {
      print('[Flutter] Getting document fields for attribute: $attribute');
      final Map<dynamic, dynamic>? result = await _channel.invokeMethod('getDocumentFields', {
        'attribute': attribute,
      });
      print('[Flutter] getDocumentFields result: $result');
      if (result != null) {
        return Map<String, dynamic>.from(result);
      }
      return {"error": "No fields retrieved"};
    } on PlatformException catch (e) {
      print('[Flutter] Failed to get document fields: ${e.message}');
      return {"error": e.message};
    } catch (e) {
      print('[Flutter] Unknown error during getDocumentFields: $e');
      return {"error": e.toString()};
    }
  }

  // Get device status
  static Future<int> checkDeviceStatus() async {
    try {
      final int result = await _channel.invokeMethod('checkDeviceStatus');
      print('[Flutter] Device status: $result');
      // Status codes:
      // 1: Device connected and initialized
      // 2: Device lost connection
      // 3: Device lost connection - need re-initialization
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to check device status: ${e.message}');
      return -100;
    } catch (e) {
      print('[Flutter] Unknown error during checkDeviceStatus: $e');
      return -200;
    }
  }

  // Complete document scanning workflow
  static Future<Map<String, dynamic>> scanDocumentComplete(int timeoutSeconds) async {
    try {
      print('[Flutter] Starting complete document scan (timeout: ${timeoutSeconds}s)');
      final Map<dynamic, dynamic>? result = await _channel.invokeMethod('scanDocumentComplete', {
        'timeoutSeconds': timeoutSeconds,
      });
      print('[Flutter] scanDocumentComplete result: $result');
      if (result != null) {
        return Map<String, dynamic>.from(result);
      }
      return {"error": "Null result from complete scan"};
    } on PlatformException catch (e) {
      print('[Flutter] Failed to complete scan: ${e.message}');
      return {"error": e.message};
    } catch (e) {
      print('[Flutter] Unknown error during scanDocumentComplete: $e');
      return {"error": e.toString()};
    }
  }

  // Complete document scanning workflow with debug mode
  static Future<Map<String, dynamic>> scanDocumentCompleteWithDebug(int timeoutSeconds, {bool enableDebug = false}) async {
    try {
      print('[Flutter] Starting complete document scan with debug (timeout: ${timeoutSeconds}s, debug: $enableDebug)');
      final Map<dynamic, dynamic>? result = await _channel.invokeMethod('scanDocumentCompleteWithDebug', {
        'timeoutSeconds': timeoutSeconds,
        'enableDebug': enableDebug,
      });
      print('[Flutter] scanDocumentCompleteWithDebug result: $result');
      if (result != null) {
        return Map<String, dynamic>.from(result);
      }
      return {"error": "Null result from debug scan"};
    } on PlatformException catch (e) {
      print('[Flutter] Failed to complete debug scan: ${e.message}');
      return {"error": e.message};
    } catch (e) {
      print('[Flutter] Unknown error during scanDocumentCompleteWithDebug: $e');
      return {"error": e.toString()};
    }
  }

  // Get formatted passport data (matches GUI display format)
  static Future<Map<String, dynamic>> getFormattedPassportData() async {
    try {
      print('[Flutter] Getting formatted passport data');
      final Map<dynamic, dynamic>? result = await _channel.invokeMethod('getFormattedPassportData');
      print('[Flutter] getFormattedPassportData result: $result');
      if (result != null) {
        return Map<String, dynamic>.from(result);
      }
      return {"error": "No formatted data available"};
    } on PlatformException catch (e) {
      print('[Flutter] Failed to get formatted passport data: ${e.message}');
      return {"error": e.message};
    } catch (e) {
      print('[Flutter] Unknown error during getFormattedPassportData: $e');
      return {"error": e.toString()};
    }
  }

  // Debug all available fields
  static Future<void> debugAllAvailableFields(int attribute) async {
    try {
      print('[Flutter] Starting debug scan for attribute: $attribute');
      await _channel.invokeMethod('debugAllAvailableFields', {
        'attribute': attribute,
      });
      print('[Flutter] Debug scan completed - check native logs for details');
    } on PlatformException catch (e) {
      print('[Flutter] Failed to debug fields: ${e.message}');
    } catch (e) {
      print('[Flutter] Unknown error during debugAllAvailableFields: $e');
    }
  }

  // Validate field data
  static Future<bool> isValidPassportNumber(String passportNum) async {
    try {
      final bool result = await _channel.invokeMethod('isValidPassportNumber', {
        'passportNumber': passportNum,
      });
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to validate passport number: ${e.message}');
      return false;
    } catch (e) {
      print('[Flutter] Unknown error during passport validation: $e');
      return false;
    }
  }

  static Future<bool> isValidDate(String dateStr) async {
    try {
      final bool result = await _channel.invokeMethod('isValidDate', {
        'dateString': dateStr,
      });
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to validate date: ${e.message}');
      return false;
    } catch (e) {
      print('[Flutter] Unknown error during date validation: $e');
      return false;
    }
  }

  static Future<bool> isValidMRZ(String mrzLine) async {
    try {
      final bool result = await _channel.invokeMethod('isValidMRZ', {
        'mrzLine': mrzLine,
      });
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to validate MRZ: ${e.message}');
      return false;
    } catch (e) {
      print('[Flutter] Unknown error during MRZ validation: $e');
      return false;
    }
  }

  // Format date from YYYYMMDD to YYYY-MM-DD
  static Future<String> formatDate(String dateStr) async {
    try {
      final String result = await _channel.invokeMethod('formatDate', {
        'dateString': dateStr,
      });
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to format date: ${e.message}');
      return dateStr; // Return original if formatting fails
    } catch (e) {
      print('[Flutter] Unknown error during date formatting: $e');
      return dateStr;
    }
  }

  // Get document name/type
  static Future<String> getDocumentName() async {
    try {
      final String result = await _channel.invokeMethod('getDocumentName');
      print('[Flutter] Document name: $result');
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to get document name: ${e.message}');
      return "";
    } catch (e) {
      print('[Flutter] Unknown error during getDocumentName: $e');
      return "";
    }
  }

  // Save captured images
  static Future<bool> saveImages(String basePath, {int imageTypes = 0x1F}) async {
    try {
      print('[Flutter] Saving images to: $basePath');
      final bool result = await _channel.invokeMethod('saveImages', {
        'basePath': basePath,
        'imageTypes': imageTypes,
      });
      print('[Flutter] Save images result: $result');
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to save images: ${e.message}');
      return false;
    } catch (e) {
      print('[Flutter] Unknown error during saveImages: $e');
      return false;
    }
  }

  // Load configuration file
  static Future<int> loadConfiguration(String configPath) async {
    configPath = "/home/kinektek/sino_scanner/build/linux/arm64/release/bundle/lib/IDCardConfig.ini";
    try {
      print('[Flutter] Loading configuration from: $configPath');
      final int result = await _channel.invokeMethod('loadConfiguration', {
        'configPath': configPath,
      });
      print('[Flutter] Load configuration result: $result');
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to load configuration: ${e.message}');
      return -100;
    } catch (e) {
      print('[Flutter] Unknown error during loadConfiguration: $e');
      return -200;
    }
  }

  // Get last error message
  static Future<String> getLastError() async {
    try {
      final String result = await _channel.invokeMethod('getLastError');
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to get last error: ${e.message}');
      return "Failed to get error: ${e.message}";
    } catch (e) {
      print('[Flutter] Unknown error during getLastError: $e');
      return "Unknown error: $e";
    }
  }

  static Future<void> releaseScanner() async {
    try {
      await _channel.invokeMethod('releaseScanner');
      print('[Flutter] Scanner release method called.');
    } on PlatformException catch (e) {
      print('[Flutter] Failed to release scanner: ${e.message}');
    } catch (e) {
      print('[Flutter] Unknown error during releaseScanner: $e');
    }
  }

  // Utility method to interpret detection results
  static String interpretDetectionResult(int result) {
    switch (result) {
      case -1:
        return "Core engine not initialized";
      case 0:
        return "No document detected";
      case 1:
        return "Document detected and placed";
      case 2:
        return "Document was removed";
      case 3:
        return "Mobile phone barcode detected";
      case -100:
        return "Platform exception occurred";
      case -200:
        return "Unknown error occurred";
      default:
        return "Unknown detection status: $result";
    }
  }

  // Utility method to interpret device status
  static String interpretDeviceStatus(int status) {
    switch (status) {
      case 1:
        return "Device connected and initialized";
      case 2:
        return "Device lost connection";
      case 3:
        return "Device lost connection - need re-initialization";
      case -100:
        return "Platform exception occurred";
      case -200:
        return "Unknown error occurred";
      default:
        return "Unknown device status: $status";
    }
  }

  // Utility method to interpret processing results
  static String interpretProcessingResult(int result) {
    if (result > 0) {
      return "Document processed successfully (Type: $result)";
    }

    switch (result) {
      case -1:
        return "No valid document types set for classification";
      case -2:
        return "Image capturing failed";
      case -3:
        return "Image cutting failed";
      case -4:
        return "Classification failed - no matched template";
      case -5:
        return "Classification failed - no valid document types";
      case -6:
        return "Classification failed - recognition rejected";
      case -7:
        return "Recognition failed";
      case -8:
        return "Chip reading failed but page recognition successful";
      case -9:
        return "Chip reading successful but page recognition failed";
      case -10:
        return "Both page recognition and chip reading failed";
      default:
        return "Unknown processing error: $result";
    }
  }

  // Utility method to interpret card type flags
  static String interpretCardType(int cardType) {
    List<String> features = [];

    if (cardType & 1 != 0) features.add("Has Chip");
    if (cardType & 2 != 0) features.add("No Chip");
    if (cardType & 4 != 0) features.add("Has Barcode");
    if (cardType & 5 != 0) features.add("Has Chip and Barcode");
    if (cardType & 6 != 0) features.add("No Chip but Has Barcode");

    return features.isNotEmpty ? features.join(", ") : "Unknown card type: $cardType";
  }

  // Enhanced passport data extraction method
  static Future<PassportData?> extractPassportData({bool useFormatted = true, bool validateData = true}) async {
    try {
      Map<String, dynamic> data;

      if (useFormatted) {
        data = await getFormattedPassportData();
      } else {
        data = await getDocumentFields(1); // OCR data
      }

      if (data.containsKey('error')) {
        print('[Flutter] Error extracting passport data: ${data['error']}');
        return null;
      }

      return PassportData.fromMap(data, validateData: validateData);
    } catch (e) {
      print('[Flutter] Exception extracting passport data: $e');
      return null;
    }
  }

  static Future<Map<String, dynamic>> savePassportImages({
    String? customName,
    int imageTypes = 0x1F,
    bool cleanupOld = true,
  }) async {
    try {
      // Get the proper image path
      String imagePath = await ImagePathHelper.getPassportImagePath(customName: customName);

      print('[Flutter] Saving passport images to: $imagePath');

      // Save images using the original method
      bool success = await SinosecuReader.saveImages(imagePath, imageTypes: imageTypes);

      Map<String, dynamic> result = {
        'success': success,
        'basePath': imagePath,
      };

      if (success) {
        // List the actual saved files
        List<String> expectedFiles = [];

        // Based on imageTypes flags, predict what files should be created
        if (imageTypes & 0x01 != 0) expectedFiles.add('${imagePath}.jpg'); // White image
        if (imageTypes & 0x02 != 0) expectedFiles.add('${imagePath}_IR.jpg'); // IR image
        if (imageTypes & 0x04 != 0) expectedFiles.add('${imagePath}_UV.jpg'); // UV image
        if (imageTypes & 0x08 != 0) expectedFiles.add('${imagePath}_Head.jpg'); // Page portrait
        if (imageTypes & 0x10 != 0) expectedFiles.add('${imagePath}_HeadEc.jpg'); // Chip portrait

        // Check which files actually exist
        List<String> savedFiles = [];
        for (String filePath in expectedFiles) {
          if (await File(filePath).exists()) {
            savedFiles.add(filePath);
          }
        }

        result['savedFiles'] = savedFiles;
        result['fileCount'] = savedFiles.length;

        print('[Flutter] Successfully saved ${savedFiles.length} image files');

        // Cleanup old images if requested
        if (cleanupOld) {
          await ImagePathHelper.cleanupOldImages();
        }

      } else {
        result['error'] = 'Failed to save images';
      }

      return result;

    } catch (e) {
      print('[Flutter] Error in savePassportImages: $e');
      return {
        'success': false,
        'error': e.toString(),
      };
    }
  }

  // Get all saved passport images
  static Future<List<String>> getAllPassportImages() async {
    return await ImagePathHelper.getPassportImagePaths();
  }

  // Helper method for complete workflow with enhanced error handling
  static Future<ScanResult> performCompleteScan({
    int timeoutSeconds = 20,
    bool enableDebug = false,
    bool validateData = true,
    bool saveImages = false,
    String? imagePath,
    customName,
    cleanupOld,

  }) async {
    try {
      print('[Flutter] Starting complete scan workflow...');

      // Check device status first
      int deviceStatus = await checkDeviceStatus();
      if (deviceStatus != 1) {
        return ScanResult.error("Device not ready: ${interpretDeviceStatus(deviceStatus)}");
      }

      // Perform the scan
      Map<String, dynamic> scanResult;
      if (enableDebug) {
        scanResult = await scanDocumentCompleteWithDebug(timeoutSeconds, enableDebug: true);
      } else {
        scanResult = await scanDocumentComplete(timeoutSeconds);
      }

      if (scanResult.containsKey('error')) {
        return ScanResult.error(scanResult['error']);
      }

      // Extract passport data
      PassportData? passportData = await extractPassportData(
        useFormatted: true,
        validateData: validateData,
      );

      // Save images if requested
      bool imagesSaved = false;
      if (saveImages && imagePath != null) {
        imagesSaved = await SinosecuReader.saveImages(imagePath);
      }

      return ScanResult.success(
        passportData: passportData,
        rawScanData: scanResult,
        imagesSaved: imagesSaved,
      );

    } catch (e) {
      return ScanResult.error("Scan workflow failed: $e");
    }
  }
}