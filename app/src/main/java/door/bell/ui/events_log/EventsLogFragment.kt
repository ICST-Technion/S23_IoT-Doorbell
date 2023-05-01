package door.bell.ui.events_log

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import door.bell.databinding.FragmentEventsLogBinding

class EventsLogFragment : Fragment() {

    private var _binding: FragmentEventsLogBinding? = null

    // This property is only valid between onCreateView and
    // onDestroyView.
    private val binding get() = _binding!!

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val eventsLogViewModel =
            ViewModelProvider(this).get(EventsLogViewModel::class.java)

        _binding = FragmentEventsLogBinding.inflate(inflater, container, false)
        val root: View = binding.root

        val textView: TextView = binding.textEventsLog
        eventsLogViewModel.text.observe(viewLifecycleOwner) {
            textView.text = it
        }
        return root
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}