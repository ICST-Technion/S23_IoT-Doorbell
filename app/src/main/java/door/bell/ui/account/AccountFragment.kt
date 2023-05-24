package door.bell.ui.account

import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import androidx.activity.result.contract.ActivityResultContracts
import androidx.fragment.app.Fragment
import com.google.android.material.snackbar.Snackbar
import com.google.firebase.firestore.FirebaseFirestore
import door.bell.R
import com.google.firebase.storage.FirebaseStorage
import com.google.firebase.storage.StorageMetadata
import com.google.firebase.storage.StorageReference
import java.util.*

class AccountFragment : Fragment() {
    private lateinit var uploadButton: Button
    private var audioUri: Uri? = null
    private val firestore = FirebaseFirestore.getInstance()
    private val storage = FirebaseStorage.getInstance()
    private val storageRef = storage.reference
    private lateinit var audioRef: StorageReference

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view = inflater.inflate(R.layout.fragment_account, container, false)
        uploadButton = view.findViewById(R.id.upload_button)
        val audioPicker =
            registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
                if (uri != null) {
                    audioUri = uri
                    uploadAudio()
                }
            }

        uploadButton.setOnClickListener {
            audioPicker.launch(arrayOf("audio/*"))
        }

        return view
    }

    override fun onDestroyView() {
        super.onDestroyView()
        audioUri = null
    }

    private fun uploadAudio() {
        // Delete previous audio document in Firestore
        firestore.collection("audio").document("message")
            .delete()
            .addOnSuccessListener {
                // Upload the new audio file to Firebase Cloud Storage
                val audioFileName = UUID.randomUUID().toString()
                audioRef = storageRef.child("audio/$audioFileName")
                val uploadTask = audioRef.putFile(audioUri!!)

                uploadTask.continueWithTask { task ->
                    if (!task.isSuccessful) {
                        throw task.exception!!
                    }

                    // Set metadata for the uploaded audio file
                    val metadata = StorageMetadata.Builder()
                        .setContentType("audio/wav")
                        .build()
                    audioRef.updateMetadata(metadata)
                }.addOnCompleteListener { task ->
                    if (task.isSuccessful) {
                        // Get the download URL of the uploaded audio file
                        audioRef.downloadUrl.addOnSuccessListener { downloadUri ->
                            val audioUrl = downloadUri.toString()
                            saveAudioUrlToFirestore(audioUrl)
                        }.addOnFailureListener { exception ->
                            Log.e("AUDIO UPLOAD", "Error getting audio download URL: ${exception.message}")
                            showUploadErrorMessage("Upload Failed. Please try again.")
                        }
                    } else {
                        // Handle unsuccessful upload
                        Log.e("AUDIO UPLOAD", "Upload Failed. Error: ${task.exception?.message}")
                        showUploadErrorMessage("Upload Failed. Error: ${task.exception?.message}")
                    }
                }
            }
            .addOnFailureListener { exception ->
                Log.e("AUDIO UPLOAD", "Error deleting previous audio document: ${exception.message}")
                showUploadErrorMessage("Upload Failed. Error: Unable to delete previous audio document.")
            }
    }

    private fun saveAudioUrlToFirestore(audioUrl: String) {
        val audioData = hashMapOf("audioUrl" to audioUrl)
        firestore.collection("audio").document("message")
            .set(audioData)
            .addOnSuccessListener {
                Log.i("AUDIO UPLOAD", "Upload Success. Document ID: message")
                showUploadSuccessMessage("Upload Succeeded")
            }
            .addOnFailureListener { exception ->
                Log.e("AUDIO UPLOAD", "Upload Failed. Error: ${exception.message}")
                showUploadErrorMessage("Upload Failed. Error: ${exception.message}")
            }
    }

    private fun showUploadSuccessMessage(message: String) {
        activity?.runOnUiThread {
            Snackbar.make(uploadButton, message, Snackbar.LENGTH_LONG).show()
        }
    }

    private fun showUploadErrorMessage(message: String) {
        activity?.runOnUiThread {
            Snackbar.make(uploadButton, message, Snackbar.LENGTH_LONG).show()
        }
    }
}
