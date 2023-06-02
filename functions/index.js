/* REMINDER: registration token should be initialized in every installation */

const functions = require("firebase-functions");
const admin = require("firebase-admin");
registrationToken = "exY3FZMyTpaXckIhN-gDU3:APA91bFJhxo40X06MoSfZPYtzPBGBqMvxMyg4b7Ev2xD4xVIDm1q30MWIs8MqaKzF6s6IuPC67qUR-HJroB3idz_1ZrsKt53ggQk8TgigatWuOoCrE1dHrrx5PRc3_IUNZqAAMUoJDR1";
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
    };

    await fileRef.setMetadata(metadata);

    const signedUrls = await fileRef.getSignedUrl({
      action: 'read',
      expires: '03-17-2025' // set the expiration date of the URL
    });

    const firebaseImageUrl = signedUrls[0];
    functions.logger.log("this is the url from firebase: ", firebaseImageUrl)

    // Send FCM notification with the image download URL
    const payload = {
      notification: {
        title: "Someone's at the door!",
        body: "Tap to see who :)"
      },
      android: {
        notification: {
            imageUrl: firebaseImageUrl
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
