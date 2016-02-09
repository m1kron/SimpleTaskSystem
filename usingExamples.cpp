#include <array>
#include <sts\private_headers\tasking\TaskAllocator.h>
#include <sts\tasking\TaskManager.h>
#include <sts\tasking\TaskHelpers.h>
#include <sts\tasking\TaskBatch.h>

// Helper function.
int CalculateItem( int item )
{
	int sum = item;
	for( int i = 0; i < 100000; ++i )
	{
		sum += ( i % 4 ) / 2;
	}

	return sum;
}

// Example of task function.
void CalcualteItemAndWriteToArray( sts::TaskContext& context )
{
	std::array<int, 200>* array_ptr = nullptr;
	int my_index = -1;

	// Read needed parameters.
	ExistingBufferWrapperReader reader( context.GetThisTask()->GetRawDataPtr(), context.GetThisTask()->GetDataSize() );
	reader.Read( array_ptr );
	reader.Read( my_index );

	// Calculate item:
	int new_item = CalculateItem( ( *array_ptr )[ my_index ] );

	// and write it back to array:
	( *array_ptr )[ my_index ] = new_item;
}

// Another example of task function.
void ArraySummer( sts::TaskContext& context )
{
	std::array<int, 200>* array_ptr = nullptr;

	// Read the array.
	ExistingBufferWrapperReader reader( context.GetThisTask()->GetRawDataPtr(), context.GetThisTask()->GetDataSize() );
	reader.Read( array_ptr );

	// Calculate sum.
	int sum = 0;
	for( int i = 0; i < array_ptr->size(); ++i )
	{
		sum += ( *array_ptr )[ i ];
	}

	// Write result.
	ExistingBufferWrapperWriter writeBuffer( context.GetThisTask()->GetRawDataPtr(), context.GetThisTask()->GetDataSize() );
	writeBuffer.Write( sum );
}

/////////////////////////////////////////////////////////////////////////////////
// MAIN
int main( int argc, char* argv[] )
{
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Example of using system to calculate items in array and then sum all of the elements in the array.
	// Example is using coroutines to build dynamic dependency trees.
	/////////////////////////////////////////////////////////////////////////////////////////////////
	{
		// Setup the system.
		sts::TaskManager manager;
		manager.Setup();

		// This is arrray that we will work on.
		std::array< int, 200 > arrayToFill = { 0 };

		// Create lambda that will process the array in parallel.
		auto array_functor = [ &arrayToFill ]( sts::TaskContext& context )
		{
			sts::TaskBatch_AutoRelease batch( context.GetTaskManager() );

			for( unsigned i = 0; i < arrayToFill.size(); ++i )
			{
				// Dynamic dependency tree, spawn task durig execution of another task:
				// Create a task that will calculate single item.
				int item = arrayToFill[ i ];
				auto item_functor = [ item ]( sts::TaskContext& context )
				{
					int calculated_item = CalculateItem( item );

					// Store item in data task data storage.
					ExistingBufferWrapperWriter writer( context.GetThisTask()->GetRawDataPtr(), context.GetThisTask()->GetDataSize() );
					writer.Write( calculated_item );

				}; ///< end of item_functor

				// Create new task using item_functor:
				sts::TaskHandle handle = context.GetTaskManager().CreateNewTask( item_functor );
				batch.Add( std::move( handle ) );
			}

			// Submit whole batch.
			bool submitted = context.GetTaskManager().SubmitTaskBatch( batch );

			// Wait until whole batch is done.
			context.WaitFor( [ &batch ] { return batch.AreAllTaskFinished(); } );

			// Get results from child tasks and calculate final sum:
			int final_sum = 0;
			for( unsigned i = 0; i < batch.GetSize(); ++i )
			{
				const sts::TaskHandle& handle = batch[ i ];
				int sum = 0;
				ExistingBufferWrapperReader read_buffer( handle->GetRawDataPtr(), handle->GetDataSize() );
				read_buffer.Read( sum );

				// Fill array with apropriate results:
				arrayToFill[ i ] = sum;
				final_sum += sum;
			}

			// Write final sum:
			ExistingBufferWrapperWriter writer( context.GetThisTask()->GetRawDataPtr(), context.GetThisTask()->GetDataSize() );
			writer.Write( final_sum );

		}; ///< end of array_functor

		// Create main task using array_functor:
		sts::TaskHandle root_task_handle = manager.CreateNewTask( array_functor );

		// Submit main task..
		bool submitted = manager.SubmitTask( root_task_handle );

		// and help processing until main task is done:
		manager.RunTasksUsingThisThreadUntil( [ &root_task_handle ] { return root_task_handle->IsFinished(); } );

		// Read the result:
		int sum = 0;
		ExistingBufferWrapperReader read_buffer( root_task_handle->GetRawDataPtr(), root_task_handle->GetDataSize() );
		read_buffer.Read( sum );

		ASSERT( sum == 10000000 );

		// Release main task:
		manager.ReleaseTask( root_task_handle );

		ASSERT( manager.AreAllTasksReleased() );
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Example of using system to calculate items in array and then sum all of the elements in the array.
	// Example is using static trees and normal function as task functions.
	/////////////////////////////////////////////////////////////////////////////////////////////////
	{
		sts::TaskManager manager;
		manager.Setup();

		// This is arrray that we will work on.
		std::array< int, 200 > arrayToFill = { 0 };

		// Prepare batch.
		sts::TaskBatch_AutoRelease batch( manager );

		// Setup root task.
		sts::TaskHandle root_task_handle = manager.CreateNewTask( &ArraySummer );
		ExistingBufferWrapperWriter writer( root_task_handle->GetRawDataPtr(), root_task_handle->GetDataSize() );
		writer.Write( &arrayToFill );

		// Build static tree:
		for( int i = 0; i < arrayToFill.size(); ++i )
		{
			sts::TaskHandle child_handle = manager.CreateNewTask( &CalcualteItemAndWriteToArray, root_task_handle );
			ExistingBufferWrapperWriter writer( child_handle->GetRawDataPtr(), child_handle->GetDataSize() );

			// Write needed data. 
			writer.Write( &arrayToFill );
			writer.Write( i );

			batch.Add( std::move( child_handle ) );
		}

		// Submit batch.
		bool submitted = manager.SubmitTaskBatch( batch );

		// Wait until task is done.
		manager.RunTasksUsingThisThreadUntil( [ &root_task_handle ] { return root_task_handle->IsFinished(); } );

		// Read the result:
		int sum = 0;
		ExistingBufferWrapperReader read_buffer( root_task_handle->GetRawDataPtr(), root_task_handle->GetDataSize() );
		read_buffer.Read( sum );

		ASSERT( sum == 10000000 );

		// Release main task:
		manager.ReleaseTask( root_task_handle );

		ASSERT( manager.AreAllTasksReleased() );
	}
}