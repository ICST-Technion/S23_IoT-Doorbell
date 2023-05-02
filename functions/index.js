/* REMINDER: registration token should be initialized in every installation */

const functions = require("firebase-functions");
const admin = require("firebase-admin");
registrationToken = "eHm59upyQhW2gNXgaV9Vc2:APA91bFRk3gJJngFMA1EpkvRiovyrS68vz-N_FtVzsuP8vds10M8nQ1QTCuMJ5D2xkoVrddD_CWKtfmSYGCcMmo4JJ98a1-1H9_sSR_5Ejq1O-xyIjnzEtGTu2d11C6KEuRmkajhc3RX";
admin.initializeApp();

exports.sendNotificationOnImageUpload = functions.storage
  .object()
  .onFinalize(async (object) => {
    const filePath = object.name;
    const contentType = object.contentType;
    const fileBucket = object.bucket;

    // Check if the uploaded file is an image
    if (!contentType.startsWith("image/")) {
      console.log("Uploaded file is not an image");
      return null;
    }

    const fileRef = admin.storage().bucket().file(filePath);
    const metadata = {
      contentType: 'image/jpeg',
      // you can add other metadata properties here
    };

    await fileRef.setMetadata(metadata);

    const signedUrls = await fileRef.getSignedUrl({
      action: 'read',
      expires: '03-17-2025' // set the expiration date of the URL
    });

    const firebaseImageUrl = signedUrls[0];
    functions.logger.log("this is the url from firebase: ", firebaseImageUrl[0])

    // Send FCM notification with the image download URL
    const payload = {
      notification: {
        title: "Someone's at the door!",
        body: "Tap to see who :)"
      },
      android: {
        notification: {
            imageUrl: firebaseImageUrl[0]
        }
      },
      data: {
        click_action: "android.intent.action.MAIN"
      },
      token: registrationToken
    };

    const response = await admin.messaging().send(payload);
    console.log("FCM notification sent:", response);

    return null;
  });
