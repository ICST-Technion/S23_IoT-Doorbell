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
            val sortedItems = listResult.items.sortedWith(Comparator { item1, item2 ->
                val name1 = item1.name.replace("-", "/")
                val name2 = item2.name.replace("-", "/")

                val timestamp1 = name1.substring(0, 16)
                val timestamp2 = name2.substring(0, 16)

                val parts1 = timestamp1.split("/")
                val parts2 = timestamp2.split("/")

                val yearComparison = parts2[0].compareTo(parts1[0])
                if (yearComparison != 0) {
                    return@Comparator yearComparison
                }

                val monthComparison = parts2[1].compareTo(parts1[1])
                if (monthComparison != 0) {
                    return@Comparator monthComparison
                }

                val dayComparison = parts2[2].substring(0, 2).compareTo(parts1[2].substring(0, 2))
                if (dayComparison != 0) {
                    return@Comparator dayComparison
                }

                parts2[2].substring(3).compareTo(parts1[2].substring(3))
            })

            for (item in sortedItems) {
                // Get the download URL for each item
                item.downloadUrl.addOnSuccessListener { uri ->
                    // Inflate the layout for the container
                    val container = LayoutInflater.from(requireContext()).inflate(R.layout.event_item, binding.eventContainer, false)
                    // Find the ImageView and TextView views within the container using findViewById()
                    val imageView = container.findViewById<ImageView>(R.id.image_view)
                    val nameTextView = container.findViewById<TextView>(R.id.timestamp_view)
                    // Load the image using Glide and set it to the ImageView
                    Glide.with(requireContext()).load(uri).into(imageView)
                    // Set the name of the image to the TextView
                    nameTextView.text = item.name
                        .replace("-", "/")
                        .replace("^", ":")
                        .replace("_", "\n")
                        .replace(".jpg", "")
                    // Add the container to the eventContainer
                    binding.eventContainer.addView(container)
                }.addOnFailureListener { exception ->
                    Log.e("Events Log", "Failed to download image", exception)
                }
            }

        }.addOnFailureListener { exception ->
            Log.e("Events Log", "Failed to list images in folder", exception)
        }

        return root
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}