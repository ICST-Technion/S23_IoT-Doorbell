package door.bell.ui.account

import android.annotation.SuppressLint
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
import door.bell.R
import okhttp3.*
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.RequestBody.Companion.toRequestBody
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.util.*

class AccountFragment : Fragment() {
    private lateinit var uploadButton: Button
    private var audioUri: Uri? = null

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view = inflater.inflate(R.layout.fragment_account, container, false)
        uploadButton = view.findViewById(R.id.upload_button)
        val audioPicker =
            registerForActivityResult(ActivityResultContracts.GetContent()) { uri: Uri? ->
                if (uri != null) {
                    uploadAudioToGitHub(uri)
                }
            }

        uploadButton.setOnClickListener {
            audioPicker.launch("audio/*")
        }

        return view
    }

    override fun onDestroyView() {
        super.onDestroyView()
        audioUri = null
    }

    private fun uploadAudioToGitHub(uri: Uri) {
        audioUri = uri
        val selectedAudio: String? = getRealPathFromURI(audioUri!!)

        val client = OkHttpClient.Builder()
            .cookieJar(CookieJar.NO_COOKIES)
            .build()

        val fileContent = activity?.contentResolver?.openInputStream(audioUri!!)?.readBytes()
        val requestBody =
            fileContent?.toRequestBody("application/octet-stream".toMediaTypeOrNull())?.let {
                MultipartBody.Builder()
                    .setType(MultipartBody.FORM)
                    .addFormDataPart("file", selectedAudio, it)
                    .build()
            }

        val request = requestBody?.let {
            Request.Builder()
                .url("https://github.com/shaharcc/IoT-Doorbell")
                .header("Authorization", "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")
                .put(it)
                .build()
        }

        if (request != null) {
            Log.i("AUDIO UPLOAD", "handling request 1")
            client.newCall(request).enqueue(object : Callback {
                override fun onFailure(call: Call, e: IOException) {
                    Log.e("AUDIO UPLOAD", "Upload Failed. error: ${e.message}")
                    activity?.runOnUiThread {
                        Snackbar.make(uploadButton, "Upload Failed. error: ${e.message}", Snackbar.LENGTH_LONG).show()
                    }
                }

                override fun onResponse(call: Call, response: Response) {
                    if (response.isSuccessful) {
                        Log.i("AUDIO UPLOAD", "Upload Success: ${response.message}")
                        activity?.runOnUiThread {
                            Snackbar.make(uploadButton, "Upload Success", Snackbar.LENGTH_LONG).show()
                        }
                    } else {
                        val responseBody = response.body?.string()
                        Log.e("AUDIO UPLOAD", "Upload Failed: ${response.message}")
                        Log.e("AUDIO UPLOAD", "Response Body: $responseBody")
                        activity?.runOnUiThread {
                            Snackbar.make(uploadButton, "Upload Failed: ${response.message}", Snackbar.LENGTH_LONG).show()}
                    }
                }
            })
        }
    }

    private fun getRealPathFromURI(uri: Uri): String? {
        val inputStream = context?.contentResolver?.openInputStream(uri)
        val tempFile = File(context?.cacheDir, "temp_audio_file")
        val outputStream = FileOutputStream(tempFile)
        Log.e("AUDIO UPLOAD", "handling request 2")
        inputStream?.use { input ->
            outputStream.use { output ->
                input.copyTo(output)
            }
        }
        return tempFile.absolutePath
    }
}
