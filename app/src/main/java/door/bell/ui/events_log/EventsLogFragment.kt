package door.bell.ui.events_log

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.bumptech.glide.Glide
import com.google.firebase.storage.FirebaseStorage
import door.bell.R
import door.bell.databinding.FragmentEventsLogBinding
import java.lang.Thread.sleep
import java.text.ParseException
import java.text.SimpleDateFormat
import java.util.Locale
import java.util.concurrent.locks.ReentrantLock

private val mutex = ReentrantLock()


class EventsLogFragment : Fragment() {

    // Declare ImageView and TextView variables as class properties
    private lateinit var imageView: ImageView
    private lateinit var timestampTextView: TextView
    private var _binding: FragmentEventsLogBinding? = null

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {

        val binding = FragmentEventsLogBinding.inflate(inflater, container, false)
        val root = binding.root

        imageView = root.findViewById(R.id.image_view)
        timestampTextView = root.findViewById(R.id.timestamp_view)

        val storage = FirebaseStorage.getInstance()
        val storageRef = storage.reference.child("data")
        val listTask = storageRef.listAll()

        listTask.addOnSuccessListener { listResult ->
            val format = SimpleDateFormat("yyyy-MM-dd_HH:mm", Locale.getDefault())
            val sortedItems = listResult.items.sortedByDescending { item ->
                val timestampString = item.name.substringBeforeLast(".")
                try {
                    val date = format.parse(timestampString)
                    Log.d("Timestamp", "Parsed date: $date")
                    date
                } catch (e: ParseException) {
                    Log.e("Timestamp", "Failed to parse date: $timestampString")
                    null
                }
            }

            for (item in sortedItems) {
                mutex.lock()
                Log.i("events item in sorted list", "item: ${item.name}")
                sleep(700)
                item.downloadUrl.addOnSuccessListener { uri ->
                    try {
                        val eventContainer = LayoutInflater.from(requireContext()).inflate(R.layout.event_item, binding.eventContainer, false)
                        val imageView = eventContainer.findViewById<ImageView>(R.id.image_view)
                        val nameTextView = eventContainer.findViewById<TextView>(R.id.timestamp_view)

                        Glide.with(requireContext()).load(uri).into(imageView)
                        nameTextView.text = item.name
                            .replace("-", "/")
                            .replace("_", "\n")
                            .replace(".jpg", "")
                        binding.eventContainer.addView(eventContainer)
                    } finally {
                        // Release the lock after modifying the view hierarchy
                        mutex.unlock()
                    }
                }.addOnFailureListener { exception ->
                    Log.e("Events Log", "Failed to download image", exception)
                }
            }
        }

        return root
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
