import 'dart:io';
import 'package:path/path.dart' as path;

class ImagePathHelper {
  // Get the project root directory
  static String getProjectRoot() {
    // Get current working directory
    String currentDir = Directory.current.path;

    // If running from build directory, go up to project root
    if (currentDir.contains('build')) {
      // Navigate up to project root
      while (!currentDir.endsWith('sino_scanner') && currentDir.contains('build')) {
        currentDir = path.dirname(currentDir);
      }
    }

    return currentDir;
  }

  // Create and get images directory
  static Future<String> getImagesDirectory() async {
    String projectRoot = getProjectRoot();
    String imagesDir = path.join(projectRoot, 'images');

    // Create directory if it doesn't exist
    Directory dir = Directory(imagesDir);
    if (!await dir.exists()) {
      await dir.create(recursive: true);
      print('[Flutter] Created images directory: $imagesDir');
    }

    return imagesDir;
  }

  // Create and get passports subdirectory
  static Future<String> getPassportsDirectory() async {
    String imagesDir = await getImagesDirectory();
    String passportsDir = path.join(imagesDir, 'passports');

    Directory dir = Directory(passportsDir);
    if (!await dir.exists()) {
      await dir.create(recursive: true);
      print('[Flutter] Created passports directory: $passportsDir');
    }

    return passportsDir;
  }

  // Generate unique filename for passport images
  static String generatePassportImagePath(String baseFileName) {
    DateTime now = DateTime.now();
    String timestamp = '${now.year}${now.month.toString().padLeft(2, '0')}${now.day.toString().padLeft(2, '0')}_'
        '${now.hour.toString().padLeft(2, '0')}${now.minute.toString().padLeft(2, '0')}${now.second.toString().padLeft(2, '0')}';

    return '${baseFileName}_$timestamp';
  }

  // Get full path for saving passport images
  static Future<String> getPassportImagePath({String? customName}) async {
    String passportsDir = await getPassportsDirectory();
    String fileName = customName ?? generatePassportImagePath('passport');

    // Don't add .jpg extension here - the SDK will add it automatically
    return path.join(passportsDir, fileName);
  }

  // Clean up old images (optional - keep only last N images)
  static Future<void> cleanupOldImages({int keepCount = 50}) async {
    try {
      String passportsDir = await getPassportsDirectory();
      Directory dir = Directory(passportsDir);

      if (await dir.exists()) {
        List<FileSystemEntity> files = await dir.list().toList();

        // Filter only image files and sort by modification time
        List<File> imageFiles = files
            .whereType<File>()
            .where((file) => file.path.toLowerCase().endsWith('.jpg') ||
            file.path.toLowerCase().endsWith('.png') ||
            file.path.toLowerCase().endsWith('.bmp'))
            .toList();

        // Sort by modification time (newest first)
        imageFiles.sort((a, b) => b.lastModifiedSync().compareTo(a.lastModifiedSync()));

        // Delete old files if we have more than keepCount
        if (imageFiles.length > keepCount) {
          for (int i = keepCount; i < imageFiles.length; i++) {
            await imageFiles[i].delete();
            print('[Flutter] Deleted old image: ${imageFiles[i].path}');
          }
        }
      }
    } catch (e) {
      print('[Flutter] Error cleaning up old images: $e');
    }
  }

  // List all saved passport images
  static Future<List<String>> getPassportImagePaths() async {
    try {
      String passportsDir = await getPassportsDirectory();
      Directory dir = Directory(passportsDir);

      if (await dir.exists()) {
        List<FileSystemEntity> files = await dir.list().toList();

        return files
            .whereType<File>()
            .where((file) => file.path.toLowerCase().endsWith('.jpg') ||
            file.path.toLowerCase().endsWith('.png') ||
            file.path.toLowerCase().endsWith('.bmp'))
            .map((file) => file.path)
            .toList();
      }
    } catch (e) {
      print('[Flutter] Error listing passport images: $e');
    }

    return [];
  }
}