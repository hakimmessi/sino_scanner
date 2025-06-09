import 'package:flutter/services.dart';

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
}